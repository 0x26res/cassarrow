import os
import pathlib
import typing

import pkg_resources
import pyarrow as pa
import pytest

import _cassarrow
import cassarrow
import cassarrow.impl
from tests.test_compare_with_json import compare_json


def get_binary(name: str) -> bytes:
    full_name = pkg_resources.resource_filename(__name__, os.path.join("select", name))
    with open(full_name, "rb") as fp:
        return fp.read()


def load_all_data(table: str) -> typing.Iterator[bytes]:
    directory = (
        pathlib.Path(pkg_resources.resource_filename(__name__, "select")) / table
    )
    for file in sorted(os.listdir(directory)):
        if file.endswith(".bin"):
            with (directory / file).open("rb") as fp:
                yield fp.read()


def load_json(table: str) -> list[str]:
    directory = (
        pathlib.Path(pkg_resources.resource_filename(__name__, "select")) / table
    )
    with (directory / "all.jsonl").open("r") as fp:
        return [line.strip() for line in fp]


@pytest.mark.parametrize(
    "table_name",
    [
        "simple_map",
        "time_series",
        "simple_primitives",
    ],
)
def test_from_dump(table_name: str):
    # table = "time_series"

    batches = []
    results = None
    for data in load_all_data(table_name):
        batch = cassarrow.impl.ArrowProtocolHandler.decode_message(
            5, {}, 3, 0, 8, data, None, []
        )
        batches.append(batch.parsed_rows)
        results = batch
    table = pa.Table.from_batches(batches)
    expected_json = load_json(table_name)

    compare_json(table, results, expected_json)


def test_from_empty():
    results = _cassarrow.parse_results(
        bytes([0x0, 0x0, 0x0, 0x0]),
        pa.schema([pa.field("int", pa.int32()), pa.field("double", pa.float64())]),
    )
    assert isinstance(results, pa.RecordBatch)
    assert results.num_rows == 0
    assert results.num_columns == 2
