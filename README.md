# Cassarrow

Arrow based Cassandra python driver. 

## TLDR;

Speed up the cassandra python driver using C++ to parse cassandra queries data as [Apache Arrow](https://arrow.apache.org/) tables.

Key features:
* 20x speed up in the parsing of results
* 14x less memory
* Support for most native types, UDT, List and Set

## Getting Started

### Installation

```shell
pip install cassarrow
```

### Usage

```python
import cassarrow
import pyarrow as pa

# ...

with cassarrow.install_cassarrow(session) as cassarrow_session:
    table: pa.Table = cassarrow.result_set_to_table(cassarrow_session.execute("SELECT * FROM my_table"))
```

## Type Mapping

### Native Types

| Cassandra   | pyarrow              | Note         |
|:------------|:---------------------|:-------------|
| ascii       | `pa.string()`        |              |
| bigint      | `pa.int64()`         |              |
| blob        | `pa.binary()`        |              |
| boolean     | `pa.bool_()`         |              |
| counter     |                      | TODO         |
| date        | `pa.date32()`        |              |
| decimal     |                      | Incompatible |
| double      | `pa.float64()`       |              |
| duration    | `pa.duration("ns")`  |              |
| float       | `pa.float32()`       |              |
| inet        |                      | TODO         |
| int         | `pa.int32()`         |              |
| smallint    | `pa.int16()`         |              |
| text        | `pa.string()`        |              |
| time        | `pa.time64("ns")`    |              |
| timestamp   | `pa.timestamp("ms")` |              |
| timeuuid    | `pa.binary(16)`      |              |
| tinyint     | `pa.int8()`          |              |
| uuid        | `pa.binary(16)`      |              |
| varchar     | `pa.string()`        |              |
| varint      |                      | Incompatible |

## Collections / UDT

| Cassandra   | pyarrow     | Note   |
|:------------|:------------|:-------|
| list        | `pa.list_`  |        |
| map         | `pa.map_`   |        |
| set         | `pa.list_`  |        |
| udt         | `pa.struct` |        |