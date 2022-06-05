import contextlib
import io
import typing

import _cassarrow
import cassandra.cluster
import pyarrow as pa  # Must be imported before _cassarrow
from cassandra.cqltypes import CassandraTypeType
from cassandra.protocol import ResultMessage, _ProtocolHandler

NATIVE_TYPES = {
    "ascii": pa.string(),
    "bigint": pa.int64(),
    "blob": pa.binary(),
    "boolean": pa.bool_(),
    "counter": pa.int64(),
    "date": pa.date32(),
    "double": pa.float64(),
    "duration": pa.duration("ns"),
    "float": pa.float32(),
    # "inet":
    "int": pa.int32(),
    "smallint": pa.int16(),
    "text": pa.string(),
    "time": pa.time64("ns"),
    "timestamp": pa.timestamp("ms"),
    "timeuuid": pa.binary(16),
    "tinyint": pa.int8(),
    "uuid": pa.binary(16),
    "varchar": pa.string(),
    # "varint":
}


def get_arrow_type(dtype: CassandraTypeType) -> pa.DataType:
    typename = dtype.typename
    cassname = dtype.cassname
    if cassname in ("ListType", "SetType"):
        assert len(dtype.subtypes) == 1
        return pa.list_(get_arrow_type(dtype.subtypes[0]))
    elif cassname == "MapType":
        assert len(dtype.subtypes) == 2
        return pa.map_(
            get_arrow_type(dtype.subtypes[0]),
            get_arrow_type(dtype.subtypes[1]),
            keys_sorted=True,
        )
    elif cassname == "UserType":
        return pa.struct(
            [
                pa.field(name, get_arrow_type(subtype))
                for name, subtype in zip(dtype.fieldnames, dtype.subtypes)
            ]
        )
    try:
        return NATIVE_TYPES[typename]
    except KeyError:
        raise TypeError(f"{typename}: {dtype}")


def column_metadata_to_schema(column_metadata: list[tuple]) -> pa.Schema:
    return pa.schema(
        [
            pa.field(column_name, get_arrow_type(dtype))
            for keyspace, table, column_name, dtype in column_metadata
        ]
    )


def metadata_to_schema(names: list[str], dtypes: list[CassandraTypeType]):
    return pa.schema(
        [pa.field(name, get_arrow_type(dtype)) for name, dtype in zip(names, dtypes)]
    )


class ArrowResultMessage(ResultMessage):
    code_to_type = dict((v, k) for k, v in ResultMessage.type_codes.items())

    def recv_results_rows(
        self,
        f: io.BytesIO,
        protocol_version: int,
        user_type_map: dict,
        result_metadata: list,
    ) -> None:
        assert protocol_version >= 3
        self.recv_results_metadata(f, user_type_map)
        column_metadata = self.column_metadata or result_metadata
        self.column_names = [c[2] for c in column_metadata]
        self.column_types = [c[3] for c in column_metadata]
        schema = column_metadata_to_schema(column_metadata)
        self.parsed_rows = _cassarrow.parse_results(f.read(), schema)


def result_set_to_table(result_set: cassandra.cluster.ResultSet) -> pa.Table:
    schema = metadata_to_schema(result_set.column_names, result_set.column_types)
    return pa.Table.from_batches(result_set, schema=schema)


class ArrowProtocolHandler(_ProtocolHandler):
    message_types_by_opcode = _ProtocolHandler.message_types_by_opcode | {
        ArrowResultMessage.opcode: ArrowResultMessage
    }


def record_batch_factory(
    colnames: list[str], rows: pa.RecordBatch
) -> tuple[pa.RecordBatch]:
    return (rows,)


@contextlib.contextmanager
def install_cassarrow(
    session: cassandra.cluster.Session,
) -> typing.Iterator[cassandra.cluster.Session]:
    row_factory, client_protocol_handler = (
        session.row_factory,
        session.client_protocol_handler,
    )
    session.row_factory, session.client_protocol_handler = (
        record_batch_factory,
        ArrowProtocolHandler,
    )
    yield session
    session.row_factory, session.client_protocol_handler = (
        row_factory,
        client_protocol_handler,
    )
