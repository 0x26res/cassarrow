import _cassarrow
import os
import pkg_resources
import pyarrow as pa

import cassarrow
import cassarrow.impl


def get_binary(name: str) -> bytes:
    full_name = pkg_resources.resource_filename(__name__, os.path.join("bodies", name))
    with open(full_name, "rb") as fp:
        return fp.read()


def test_from_dump():
    # TODO: document how to save the binary file / write script to extract binary
    # TODO: extract more binaries and compare with CSVs
    data = get_binary("5.bin")
    msg_arrow = cassarrow.impl.ArrowProtocolHandler.decode_message(5, {}, 3, 0, 8, data, None, [])
    assert isinstance(msg_arrow.parsed_rows, pa.RecordBatch)
    assert msg_arrow.parsed_rows.num_rows == 5000
    assert msg_arrow.parsed_rows.num_columns == 4


def test_from_empty():
    results = _cassarrow.parse_results(
        bytes([0x0, 0x0, 0x0, 0x0]), pa.schema([pa.field("int", pa.int32()), pa.field("double", pa.float64())])
    )
    assert isinstance(results, pa.RecordBatch)
    assert results.num_rows == 0
    assert results.num_columns == 2
