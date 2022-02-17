import cassandra.cluster
import cassandra.cqltypes
import numpy as np
from cassandra.protocol import NumpyProtocolHandler
from cassandra.query import tuple_factory

import cassarrow


def test_query(session: cassandra.cluster.Session):
    results = session.execute("SELECT * FROM cassarrow.time_series WHERE event_date = '2019-10-01' LIMIT  10")
    assert results.column_names == ["event_date", "instrument_id", "event_timestamp", "value"]
    assert results.column_types == [
        cassandra.cqltypes.SimpleDateType,
        cassandra.cqltypes.Int32Type,
        cassandra.cqltypes.DateType,
        cassandra.cqltypes.DoubleType,
    ]
    rows = [r for r in results]
    assert len(rows) == 10
    for r in rows:
        assert isinstance(r, tuple)


def test_query_arrow(cassarrow_session: cassandra.cluster.Session):
    results = cassarrow_session.execute("SELECT * FROM cassarrow.time_series WHERE event_date = '2019-10-02' LIMIT 10")
    table = cassarrow.result_set_to_table(results)
    assert table.num_rows == 10
    assert table.num_columns == 4


def test_query_arrow_empty(cassarrow_session: cassandra.cluster.Session):
    results = cassarrow_session.execute("SELECT * FROM cassarrow.time_series WHERE event_date = '2010-10-01'")
    table = cassarrow.result_set_to_table(results)
    assert table.num_rows == 0
    assert table.num_columns == 4


def test_numpy_query(session: cassandra.cluster.Session):
    session.row_factory = tuple_factory
    session.client_protocol_handler = NumpyProtocolHandler

    results = session.execute("SELECT * FROM cassarrow.time_series WHERE event_date = '2019-10-01'")
    for batch in results:
        assert isinstance(batch, dict)
        for col, values in batch.items():
            assert isinstance(col, str)
            assert isinstance(values, (np.ndarray, np.ma.MaskedArray))
