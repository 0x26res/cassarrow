import datetime
import json

import bindings
import cassandra.cluster
import pkg_resources
import pyarrow as pa
import pytest
import pandas as pd
import typing
from cassandra.protocol import (
    NumpyProtocolHandler,
    _message_types_by_opcode,
    _ProtocolHandler,
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


def dump_timedelta(timedelta: pd.Timedelta) -> str:
    components = timedelta.components
    year = components.days // 360
    month = (components.days - year * 360) // 30
    day = components.days - year * 360 - month * 30
    return f"{year}y{month}mo{day}d{components.hours}h{components.minutes}m{components.seconds}s"


def dump_default(value: typing.Any) -> str:
    if isinstance(value, datetime.datetime):
        return value.isoformat(sep=" ", timespec="milliseconds") + "Z"
    elif isinstance(value, datetime.time):
        return value.isoformat() + "000"
    elif isinstance(value, datetime.date):
        return value.isoformat()
    elif isinstance(value, pd.Timedelta):
        return dump_timedelta(value)
    else:
        raise TypeError(type(value))


def compare_query_results(session: cassandra.cluster.Session, query: str):
    assert query.startswith("SELECT")
    json_results = session.execute(query.replace("SELECT ", "SELECT JSON "))
    json_records_expected: list[str] = [r.json for r in json_results]

    session.client_protocol_handler = ArrowProtocolHandler
    session.row_factory = record_batch_factory
    arrow_results = session.execute(query)
    schema = metadata_to_schema(arrow_results.column_names, arrow_results.column_types)
    rows = [r for r in arrow_results]
    table = pa.Table.from_batches(rows, schema=schema)
    json_records_actual = [
        json.dumps(r, default=dump_default) for r in table.to_pylist()
    ]
    assert len(json_records_actual) == len(json_records_expected)
    for i in range(len(json_records_actual)):
        actual = json.loads(json_records_actual[i])
        expected = json.loads(json_records_expected[i])
        assert actual == expected
        print(json_records_expected[i])


def test_query_arrow_against_json(session: cassandra.cluster.Session):
    compare_query_results(
        session, "SELECT * from cassarrow.time_series where event_date = '2019-10-02'"
    )


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


def test_query_arrow_simple_primitives(session: cassandra.cluster.Session):
    session.client_protocol_handler = ArrowProtocolHandler
    session.row_factory = record_batch_factory
    results = session.execute(
        "SELECT * from cassarrow.simple_primitives where partition_key = 'test'"
    )
    schema = metadata_to_schema(results.column_names, results.column_types)
    rows = [r for r in results]
    print(len(rows[0]))
    table = pa.Table.from_batches(rows, schema=schema)
    print(len(table))
    df = table.to_pandas()

    print(df.iloc[0].to_markdown())
    print(df.iloc[-1].to_markdown())


def test_query_arrow_simple_primitives_compare(session: cassandra.cluster.Session):
    columns = [
        "ascii_col",
        "bigint_col",
        # "blob_col",
        "boolean_col",
        "date_col",
        "double_col",
        "duration_col",
        # "float_col",
        "int_col",
        "partition_key",
        "secondary_key",
        "smallint_col",
        "text_col",
        "time_col",
        "timestamp_col",
        "tinyint_col",
        "varchar_col",
    ]

    query = f"SELECT {', '.join(columns)} from cassarrow.simple_primitives where partition_key = 'test'"
    compare_query_results(session, query)


def test_query_arrow_simple_primitives_list_compare(session: cassandra.cluster.Session):
    columns = [
        "ascii_col",
        "bigint_col",
        # "blob_col",
        "boolean_col",
        "date_col",
        "double_col",
        "duration_col",
        # "float_col",
        "int_col",
        "partition_key",
        "secondary_key",
        "smallint_col",
        "text_col",
        "time_col",
        "timestamp_col",
        "tinyint_col",
        "varchar_col",
    ]

    query = f"SELECT {', '.join(columns)} from cassarrow.simple_list where partition_key = 'test'"
    compare_query_results(session, query)


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
