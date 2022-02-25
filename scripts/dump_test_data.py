import pathlib

import cassandra
from cassandra.protocol import _ProtocolHandler
import cassandra.cluster


TO_SAVE = {
    "time_series": "SELECT * FROM cassarrow.time_series WHERE event_date = '2019-10-02'",
    "simple_primitives": "SELECT * FROM cassarrow.simple_primitives",
}


def create_dump_protocol_handler(destination: pathlib.Path):
    class DumpProtocolHandler(_ProtocolHandler):
        @classmethod
        def decode_message(
            cls, protocol_version, user_type_map, stream_id, flags, opcode, body, decompressor, result_metadata
        ):
            stream_destination = destination / f"{stream_id}.bin"
            with stream_destination.open("wb") as fp:
                fp.write(body)
            print(f"Saved {len(body)} to {stream_destination}")

            return _ProtocolHandler.decode_message(
                protocol_version, user_type_map, stream_id, flags, opcode, body, decompressor, result_metadata
            )

    return DumpProtocolHandler


def dump_query(destination: pathlib.Path, query: str):
    cluster = cassandra.cluster.Cluster()
    with cluster.connect("cassarrow") as connection:
        connection.client_protocol_handler = create_dump_protocol_handler(destination)
        connection.execute(query)


def dump_all():
    for destination, query in TO_SAVE.items():
        actual_destination = pathlib.Path("tests/data") / destination
        actual_destination.mkdir(parents=True, exist_ok=True)
        dump_query(actual_destination, query)


if __name__ == "__main__":
    dump_all()
