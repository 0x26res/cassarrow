# Cassarrow

Arrow based Cassandra python driver

## Getting Started

### Installation

```shell
pip install cassarrow
```

### Usage

```python
import cassarrow

# ...

with cassarrow.install_cassarrow(session) as cassarrow_session:
    table = cassarrow.result_set_to_table(cassarrow_session.execute_query("SELECT * FROM my_table"))
```

## Type Mapping

### Native Types

| Cassandra   | pyarrow              | Note          |
|:------------|:---------------------|:--------------|
| ascii       | `pa.string()`        |               |
| bigint      | `pa.int64()`         |               |
| blob        | `pa.binary()`        |               |
| boolean     | `pa.bool_()`         |               |
| counter     |                      | Not supported |
| date        | `pa.date32()`        |               |
| decimal     |                      | Not supported |
| double      | `pa.float64()`       |               |
| duration    | `pa.duration("ns")`  |               |
| float       | `pa.float32()`       |               |
| inet        |                      | Not supported |
| int         | `pa.int32()`         |               |
| smallint    | `pa.int16()`         |               |
| text        | `pa.string()`        |               |
| time        | `pa.time64("ns")`    |               |
| timestamp   | `pa.timestamp("ms")` |               |
| timeuuid    | `pa.binary(16)`      |               |
| tinyint     | `pa.int8()`          |               |
| uuid        | `pa.binary(16)`      |               |
| varchar     | `pa.string()`        |               |
| varint      |                      | Not supported |


## Collections / UDT

| Cassandra   | pyarrow     | Note          |
|:------------|:------------|:--------------|
| list        | `pa.list_`  |               |
| map         |             | Not supported |
| set         |             | Not supported |
| udt         | `pa.struct` |               |
