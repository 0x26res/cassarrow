import cassandra.cluster
import pytest
import typing

import cassarrow


@pytest.fixture()
def cluster() -> cassandra.cluster.Cluster:
    return cassandra.cluster.Cluster()


@pytest.fixture()
def session(cluster: cassandra.cluster.Cluster) -> typing.Iterator[cassandra.cluster.Session]:
    with cluster.connect("cassarrow") as s:
        yield s


@pytest.fixture()
def cassarrow_session(session: cassandra.cluster.Session) -> typing.Iterator[cassandra.cluster.Session]:
    with cassarrow.install_cassarrow(session) as s:
        yield s
