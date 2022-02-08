import bindings
import cassandra.cluster
import pkg_resources
import pyarrow as pa
import pytest
from cassandra.cqltypes import SimpleDateType
from cassandra.protocol import (
    NumpyProtocolHandler,
    _ProtocolHandler,
    _message_types_by_opcode,
)
from cassandra.query import tuple_factory

from cassarrow import (
    ArrowProtocolHandler,
    ArrowResultMessage,
    metadata_to_schema,
    record_batch_factory,
)


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
    # session.client_protocol_handler = ArrowProtocolHandler
    results = session.execute(
        "SELECT * from cassarrow.time_series where event_date = '2019-10-01'"
    )
    rows = [r for r in results]
    print(results.column_names)
    print(results.column_types)
    print(len(rows))
    print(rows[:5])


def test_query_arrow(session: cassandra.cluster.Session):
    session.client_protocol_handler = ArrowProtocolHandler
    session.row_factory = record_batch_factory
    results = session.execute(
        "SELECT * from cassarrow.time_series where event_date = '2019-10-02'"
    )
    schema = metadata_to_schema(results.column_names, results.column_types)
    rows = [r for r in results]
    print(len(rows[0]))
    table = pa.Table.from_batches(rows, schema=schema)
    print(len(table))
    print(table.to_pandas().head().to_markdown())
    print(table.to_pandas().tail().to_markdown())


def test_query_arrow_empty(session: cassandra.cluster.Session):
    session.client_protocol_handler = ArrowProtocolHandler
    session.row_factory = record_batch_factory
    results = session.execute(
        "SELECT * from cassarrow.time_series where event_date = '2010-10-01'"
    )
    schema = metadata_to_schema(results.column_names, results.column_types)
    rows = [r for r in results]
    table = pa.Table.from_batches(rows, schema=schema)
    assert len(table) == 0


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
    print(results.column_names)
    print(results.column_types)


def test_from_data():
    file_name = pkg_resources.resource_filename(__name__, "bodies/5.bin")

    with open(file_name, "rb") as fp:
        data = fp.read()

    msg_classic = _ProtocolHandler.decode_message(5, {}, 3, 0, 8, data, None, [])

    _ProtocolHandler.message_types_by_opcode[8] = ArrowResultMessage
    msg_arrow = _ProtocolHandler.decode_message(5, {}, 3, 0, 8, data, None, [])

    print(msg_arrow.parsed_rows)


def test_bindings():
    bindings.parse_results(
        bytes([0x0, 0x0, 0x0, 0x0]),
        pa.schema(
            [
                pa.field("int", pa.int32()),
                pa.field("double", pa.float64()),
            ]
        ),
    )
