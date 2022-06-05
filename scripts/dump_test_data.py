import pathlib

import cassandra
import cassandra.cluster
from cassandra.protocol import _ProtocolHandler

SELECT_QUERIES = {
    "time_series": (
        "SELECT * FROM cassarrow.time_series WHERE event_date = '2019-10-02'"
    ),
    "simple_primitives": "SELECT * FROM cassarrow.simple_primitives",
    "simple_map": "SELECT * FROM cassarrow.simple_map",
}


def create_dump_select_protocol_handler(destination: pathlib.Path):
    class DumpProtocolHandler(_ProtocolHandler):
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
            stream_destination = destination / f"{stream_id:04}.bin"
            with stream_destination.open("wb") as fp:
                fp.write(body)
            print(f"Saved {len(body)} to {stream_destination}")

            return super().decode_message(
                protocol_version,
                user_type_map,
                stream_id,
                flags,
                opcode,
                body,
                decompressor,
                result_metadata,
            )

    return DumpProtocolHandler


def dump_select_query(destination: pathlib.Path, query: str):
    cluster = cassandra.cluster.Cluster()
    with cluster.connect("cassarrow") as connection:

        assert query.startswith("SELECT *")
        json_query = query.replace("SELECT *", "SELECT JSON *")

        json = connection.execute(json_query)
        with (destination / "all.jsonl" "").open("w") as fp:
            for payload in json:
                fp.write(payload.json)
                fp.write("\n")

        connection.client_protocol_handler = create_dump_select_protocol_handler(
            destination
        )
        results = connection.execute(query)
        print(destination, len(list(results)))


def dump_all():
    for destination, query in SELECT_QUERIES.items():
        actual_destination = pathlib.Path("tests/select") / destination
        actual_destination.mkdir(parents=True, exist_ok=True)
        dump_select_query(actual_destination, query)


if __name__ == "__main__":
    dump_all()
