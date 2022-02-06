import io

import pyarrow as pa
import cassandra.cluster
import pkg_resources
import pytest
from cassandra.cqltypes import CassandraTypeType

from cassandra.protocol import (
    NumpyProtocolHandler,
    _ProtocolHandler,
    _message_types_by_opcode,
    ResultMessage,
    read_int,
    read_value,
)
from cassandra.query import tuple_factory


@pytest.fixture()
def cluster() -> cassandra.cluster.Cluster:
    return cassandra.cluster.Cluster()


@pytest.fixture()
def session(cluster: cassandra.cluster.Cluster) -> cassandra.cluster.Session:
    with cluster.connect("cassarrow") as s:
        yield s


def test_check_all_good():
    assert NumpyProtocolHandler is not None


class DebugProtocolHandler(_ProtocolHandler):
    @classmethod
    def encode_message(
        cls,
        msg: cassandra.protocol.QueryMessage,
        stream_id: int,
        protocol_version: int,
        compressor: None,
        allow_beta_protocol_version: bool,
    ):
        # print(type(msg))
        # print(type(stream_id))
        # print(type(protocol_version))
        # print(type(compressor))
        # print(type(allow_beta_protocol_version))
        return _ProtocolHandler.encode_message(
            msg, stream_id, protocol_version, compressor, allow_beta_protocol_version
        )

    @classmethod
    def decode_message(
        cls,
        protocol_version,
        user_type_map,
        stream_id,
        flags,
        opcode,
        body,
        decompressor,
        result_metadata,
    ):
        print("_" * 10)
        print(type(protocol_version), protocol_version)
        print(type(user_type_map), user_type_map)
        print(type(stream_id), stream_id)
        print(type(flags), flags)
        print(type(opcode), opcode)
        print(type(body), len(body))
        print(type(decompressor))
        print(type(result_metadata), result_metadata)
        # with open(str(stream_id) + ".bin", "wb") as fp:
        #     fp.write(body)

        return _ProtocolHandler.decode_message(
            protocol_version,
            user_type_map,
            stream_id,
            flags,
            opcode,
            body,
            decompressor,
            result_metadata,
        )


def test_query(session: cassandra.cluster.Session):
    session.client_protocol_handler = DebugProtocolHandler
    results = session.execute(
        "SELECT * from cassarrow.time_series where event_date = '2019-10-01'"
    )
    rows = [r for r in results]
    print(len(rows))


def test_message_types_by_opcode():
    for k, v in _message_types_by_opcode.items():
        print(k, v)


def test_numpy_query(session: cassandra.cluster.Session):

    session.row_factory = tuple_factory
    session.client_protocol_handler = NumpyProtocolHandler

    results = session.execute(
        "SELECT * from cassarrow.time_series where event_date = '2019-10-01'"
    )
    np_batches = [b for b in results]
    len(np_batches)
    print(len(np_batches))


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
    "timestamp": pa.timestamp("ns"),
    # "timeuuid"
    "tinyint": pa.int8(),
    # "uuid"
    "varchar": pa.string(),
    # "varint":
}


def get_arrow_type(dtype: CassandraTypeType) -> pa.DataType:
    typename = dtype.typename
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


def receive_pyarrow(row_count: int, f: io.BytesIO):
    for row in row_count:
        row_bytes = read_value(f)


class MyResultMessage(ResultMessage):
    """
    Cython version of Result Message that has a faster implementation of
    recv_results_row.
    """

    # type_codes = ResultMessage.type_codes.copy()
    code_to_type = dict((v, k) for k, v in ResultMessage.type_codes.items())

    def recv_results_rows(self, f, protocol_version, user_type_map, result_metadata):
        print(type(f))

        self.recv_results_metadata(f, user_type_map)
        column_metadata = self.column_metadata or result_metadata
        schema = column_metadata_to_schema(column_metadata)
        print(schema)

        rowcount = read_int(f)
        rows = [self.recv_row(f, len(column_metadata)) for _ in range(rowcount)]
        self.column_names = [c[2] for c in column_metadata]
        self.column_types = [c[3] for c in column_metadata]
        try:
            self.parsed_rows = [
                tuple(
                    ctype.from_binary(val, protocol_version)
                    for ctype, val in zip(self.column_types, row)
                )
                for row in rows
            ]
        except Exception:
            for row in rows:
                for i in range(len(row)):
                    try:
                        self.column_types[i].from_binary(row[i], protocol_version)
                    except Exception as e:
                        raise RuntimeError(
                            'Failed decoding result column "%s" of type %s: %s'
                            % (
                                self.column_names[i],
                                self.column_types[i].cql_parameterized_type(),
                                str(e),
                            )
                        )


def test_from_data():
    file_name = pkg_resources.resource_filename(__name__, "bodies/5.bin")

    with open(file_name, "rb") as fp:
        data = fp.read()

    msg = _ProtocolHandler.decode_message(5, {}, 3, 0, 8, data, None, [])
    # print(type(msg), msg)

    _ProtocolHandler.message_types_by_opcode[8] = MyResultMessage
    _ProtocolHandler.decode_message(5, {}, 3, 0, 8, data, None, [])
    # print(type(msg), msg)


def test_bindings():
    import bindings

    print(bindings.test_arrow())
    bindings.parse_results(
        b"",
        pa.schema(
            [
                pa.field("int", pa.int32()),
                pa.field("double", pa.float64()),
            ]
        ),
    )
