import io

import pyarrow as pa
import bindings
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
    # "decimal": ???
    "duration": pa.duration("ns"),
    "float": pa.float32(),
    "inet": pa.string(),
    "int": pa.int32(),
    "smallint": pa.int16(),
    "text": pa.string(),
    "time": pa.time64("ns"),
    "timestamp": pa.timestamp("ms"),
    # "timeuuid"
    "tinyint": pa.int8(),
    # "uuid"
    "varchar": pa.string(),
    # "varint":
}


def get_arrow_type(dtype: CassandraTypeType) -> pa.DataType:
    typename = dtype.typename
    if typename == "list":
        assert len(dtype.subtypes) == 1
        return pa.list_(get_arrow_type(dtype.subtypes[0]))
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
        self, f: io.BytesIO, protocol_version, user_type_map, result_metadata
    ):

        self.recv_results_metadata(f, user_type_map)
        column_metadata = self.column_metadata or result_metadata
        self.column_names = [c[2] for c in column_metadata]
        self.column_types = [c[3] for c in column_metadata]
        schema = column_metadata_to_schema(column_metadata)
        self.parsed_rows = bindings.parse_results(f.read(), schema)


class ArrowProtocolHandler(_ProtocolHandler):
    message_types_by_opcode = _ProtocolHandler.message_types_by_opcode | {
        ArrowResultMessage.opcode: ArrowResultMessage
    }


def record_batch_factory(
    colnames: list[str], rows: pa.RecordBatch
) -> tuple[pa.RecordBatch]:
    return (rows,)
