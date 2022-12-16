from typing import List

import cassandra.protocol

import cassarrow
import cassarrow.impl
from tests.test_from_binary import load_all_data


def run_with_cassarrow(data: List[bytes]):
    for d in data:
        cassarrow.impl.ArrowProtocolHandler.decode_message(5, {}, 3, 0, 8, d, None, [])


def run_with_cassandra(data: List[bytes]):
    for d in data:
        cassandra.protocol._ProtocolHandler.decode_message(5, {}, 3, 0, 8, d, None, [])


def test_benchmark_cassarrow(benchmark):
    data = list(load_all_data("time_series"))
    benchmark(run_with_cassarrow, data)


def test_benchmark_cassandra(benchmark):
    data = list(load_all_data("time_series"))
    benchmark(run_with_cassandra, data)
