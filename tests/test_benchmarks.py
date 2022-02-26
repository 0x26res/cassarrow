import cassandra.protocol
import cassandra.protocol

import cassarrow
import cassarrow.impl
import cassarrow.impl
from tests.test_from_binary import load_all_data


def run_with_cassarrow(data: list[bytes]):
    for d in data:
        cassarrow.impl.ArrowProtocolHandler.decode_message(5, {}, 3, 0, 8, d, None, [])


def run_with_cassandra(data: list[bytes]):
    for d in data:
        cassandra.protocol._ProtocolHandler.decode_message(5, {}, 3, 0, 8, d, None, [])


def test_benchmark_cassarrow(benchmark):
    data = list(load_all_data("time_series"))
    benchmark(run_with_cassarrow, data)


def test_benchmark_cassandra(benchmark):
    data = list(load_all_data("time_series"))
    benchmark(run_with_cassandra, data)
