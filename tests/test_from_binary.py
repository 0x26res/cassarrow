import pathlib

import _cassarrow
import os
import pkg_resources
import pyarrow as pa
import pytest

import cassarrow
import cassarrow.impl
from tests.test_compare_with_json import compare_json


def get_binary(name: str) -> bytes:
    full_name = pkg_resources.resource_filename(__name__, os.path.join("data", name))
    with open(full_name, "rb") as fp:
        return fp.read()


def get_binary_files(directory: pathlib.Path) -> list[pathlib.Path]:
    ints = []
    for file in os.listdir(directory):
        if file.endswith(".bin"):
            ints.append(int(file.split(".")[0]))
    ints = sorted(ints)
    return [directory / (str(file) + ".bin") for file in ints]


@pytest.mark.parametrize("table", ["time_series", "simple_primitives"])
def test_from_dump(table):
    # table = "time_series"
    directory = pathlib.Path(pkg_resources.resource_filename(__name__, "data")) / table
    batches = []
    results = None
    for file in sorted(os.listdir(directory)):
        if file.endswith(".bin"):
            with (directory / file).open("rb") as fp:
                data = fp.read()
                batch = cassarrow.impl.ArrowProtocolHandler.decode_message(5, {}, 3, 0, 8, data, None, [])
                batches.append(batch.parsed_rows)
                results = batch
    table = pa.Table.from_batches(batches)

    expected_json = []
    with (directory / "all.jsonl").open("r") as fp:
        for line in fp:
            expected_json.append(line.strip())

    compare_json(table, results, expected_json)


def test_from_empty():
    results = _cassarrow.parse_results(
        bytes([0x0, 0x0, 0x0, 0x0]), pa.schema([pa.field("int", pa.int32()), pa.field("double", pa.float64())])
    )
    assert isinstance(results, pa.RecordBatch)
    assert results.num_rows == 0
    assert results.num_columns == 2
