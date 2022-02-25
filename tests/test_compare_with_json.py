import json
import typing
import uuid

import cassandra.cluster
import cassandra.cqltypes
import pandas as pd
import pyarrow as pa
from cassandra.protocol import _ProtocolHandler

import cassarrow
from cassarrow import metadata_to_schema


def format_timedelta(timedelta: pd.Timedelta) -> str:
    components = timedelta.components
    year = components.days // 360
    month = (components.days - year * 360) // 30
    day = components.days - year * 360 - month * 30
    values = [
        (year, "y"),
        (month, "mo"),
        (day, "d"),
        (components.hours, "h"),
        (components.minutes, "m"),
        (components.seconds, "s"),
    ]

    while values and values[0][0] == 0:
        values = values[1:]
    while values and values[-1][0] == 0:
        values = values[:-1]

    return "".join([str(value) + unit for value, unit in values])


def prepare_value_for_dump(
    value: typing.Any, dtype: typing.Union[type, cassandra.cqltypes.CassandraTypeType]
) -> typing.Any:
    if value is None:
        return None
    elif dtype in (
        cassandra.cqltypes.Int32Type,
        cassandra.cqltypes.DoubleType,
        cassandra.cqltypes.LongType,
        cassandra.cqltypes.ShortType,
        cassandra.cqltypes.ByteType,
        cassandra.cqltypes.VarcharType,
        cassandra.cqltypes.AsciiType,
        cassandra.cqltypes.BooleanType,
    ):
        return value
    elif dtype is cassandra.cqltypes.FloatType:
        return float(format(value, ".3f"))
    elif dtype is cassandra.cqltypes.SimpleDateType:
        return value.strftime("%Y-%m-%d")
    elif dtype is cassandra.cqltypes.DateType:
        return value.isoformat(sep=" ", timespec="milliseconds") + "Z"
    elif dtype is cassandra.cqltypes.DurationType:
        return format_timedelta(value)
    elif dtype is cassandra.cqltypes.TimeType:
        return value.isoformat() + "000"
    elif dtype is cassandra.cqltypes.UUIDType:
        return str(uuid.UUID(bytes=value))
    elif dtype is cassandra.cqltypes.BytesType:
        return "0x" + value.hex()
    elif isinstance(dtype, cassandra.cqltypes.CassandraTypeType):
        if dtype.cassname == "UserType":
            return prepare_record_for_dump(value, dtype.fieldnames, dtype.subtypes)
        elif dtype.cassname in ("ListType", "SetType"):
            return [prepare_value_for_dump(sub_value, dtype.subtypes[0]) for sub_value in value]

    raise TypeError(f"Not supported {dtype}")


def prepare_record_for_dump(record: dict, names, types) -> dict:
    return {name: prepare_value_for_dump(record[name], dtype) for name, dtype in zip(names, types)}


class DebugProtocolHandler(_ProtocolHandler):
    """Helper to understand protocol handler's behavior/API"""

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
        cls, protocol_version, user_type_map, stream_id, flags, opcode, body, decompressor, result_metadata
    ):
        return _ProtocolHandler.decode_message(
            protocol_version, user_type_map, stream_id, flags, opcode, body, decompressor, result_metadata
        )


def compare_json(table, results, json_records_expected: list[str]):
    json_records_actual = [
        json.dumps(prepare_record_for_dump(r, results.column_names, results.column_types)) for r in table.to_pylist()
    ]
    assert len(json_records_actual) == len(json_records_expected)
    for i in range(len(json_records_actual)):
        actual = json.loads(json_records_actual[i])
        expected = json.loads(json_records_expected[i])
        assert actual == expected


def compare_query_results(session: cassandra.cluster.Session, query: str):
    assert query.startswith("SELECT")
    json_results = session.execute(query.replace("SELECT ", "SELECT JSON "))
    json_records_expected: list[str] = [r.json for r in json_results]

    with cassarrow.install_cassarrow(session) as cassarrow_session:
        arrow_results = cassarrow_session.execute(query)
        schema = metadata_to_schema(arrow_results.column_names, arrow_results.column_types)
        rows = [r for r in arrow_results]
        table = pa.Table.from_batches(rows, schema=schema)

    compare_json(table, arrow_results, json_records_expected)


def test_query_arrow_against_json(session: cassandra.cluster.Session):
    compare_query_results(session, "SELECT * FROM cassarrow.time_series WHERE event_date = '2019-10-02'")


def test_query_arrow_simple_primitives(session: cassandra.cluster.Session):
    compare_query_results(session, "SELECT * FROM cassarrow.simple_primitives")


def test_query_arrow_simple_primitives_compare(session: cassandra.cluster.Session):
    query = f"SELECT * FROM cassarrow.simple_primitives"
    compare_query_results(session, query)


def test_query_arrow_simple_list_compare(session: cassandra.cluster.Session):
    query = f"SELECT * FROM cassarrow.simple_list"
    compare_query_results(session, query)


def test_query_arrow_simple_set_compare(session: cassandra.cluster.Session):
    query = f"SELECT * FROM cassarrow.simple_set"
    compare_query_results(session, query)


def test_query_arrow_udt(session: cassandra.cluster.Session):
    query = f"SELECT * FROM cassarrow.cyclist_stats "
    compare_query_results(session, query)
