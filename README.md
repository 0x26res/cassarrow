[![PyPI Version][pypi-image]][pypi-url]
[![Python Version][versions-image]][versions-url]
[![Github Stars][stars-image]][stars-url]
[![Build Status][build-image]][build-url]
[![License][license-image]][license-url]


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



[pypi-image]: https://img.shields.io/pypi/v/cassarrow
[pypi-url]: https://pypi.org/project/cassarrow/
[build-image]: https://github.com/0x26res/cassarrow/actions/workflows/build.yaml/badge.svg
[build-url]: https://github.com/0x26res/cassarrow/actions/workflows/build.yaml
[stars-image]: https://img.shields.io/github/stars/0x26res/cassarrow
[stars-url]: https://github.com/0x26res/cassarrow
[versions-image]: https://img.shields.io/pypi/pyversions/cassarrow
[versions-url]: https://pypi.org/project/cassarrow/
[license-image]: http://img.shields.io/:license-Apache%202-blue.svg
[license-url]: https://github.com/0x26res/cassarrow/blob/master/LICENSE.txt
