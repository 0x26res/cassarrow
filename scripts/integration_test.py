import cassandra.cluster
import pyarrow as pa

import cassarrow
import cassarrow.impl

QUERY = "SELECT * FROM cassarrow.time_series WHERE event_date = '2019-10-02'"


def execute_cassarrow(session, query):
    with cassarrow.install_cassarrow(session) as cassarrow_session:
        results = cassarrow_session.execute(query)
    return cassarrow.result_set_to_table(results)


if __name__ == "__main__":
    cluster = cassandra.cluster.Cluster()
    with cluster.connect("cassarrow") as session:
        table = execute_cassarrow(session, QUERY)
        assert isinstance(table, pa.Table)
        print("LOOKS OK")
