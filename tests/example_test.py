import pytest
from cassandra.cluster import Cluster
from cassandra.protocol import NumpyProtocolHandler
from cassandra.query import tuple_factory


@pytest.fixture()
def cluster():
    return Cluster()


@pytest.fixture()
def session(cluster):
    with cluster.connect("cassarrow") as s:
        yield s


def test_check_all_good():
    assert NumpyProtocolHandler is not None


def test_query(session):

    results = session.execute(
        "SELECT * from cassarrow.time_series where event_date = '2019-10-01'"
    )
    rows = [r for r in results]
    print(len(rows))


def test_numpy_query(session):

    session.row_factory = tuple_factory
    session.client_protocol_handler = NumpyProtocolHandler

    results = session.execute(
        "SELECT * from cassarrow.time_series where event_date = '2019-10-01'"
    )
    np_batches = [b for b in results]
    len(np_batches)
    print(len(np_batches))
