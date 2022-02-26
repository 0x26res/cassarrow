# Development

## Cassandra commands

```shell
docker pull cassandra
docker network create cassandra

docker run --rm --detach \
  --name cassandra \
  --hostname cassandra \
  --network cassandra \
  --publish 127.0.0.1:9042:9042 \
  --publish 127.0.0.1:9160:9160 \
  --volume ${HOME}/data/cassandra:/var/lib/cassandra \
  cassandra

docker run --rm -it --network cassandra nuvo/docker-cqlsh cqlsh cassandra 9042 --cqlversion='3.4.5'
docker logs --follow cassandra

docker run --rm \
  --network cassandra \
  --volume "$(pwd)/migration.cql:/scripts/data.cql" \
  --env CQLSH_HOST=cassandra \
  --env CQLSH_PORT=9042 \
  --env CQLVERSION=3.4.5 \
  nuvo/docker-cqlsh

```

## Arrow Installation

See https://arrow.apache.org/docs/developers/python.html#using-pip

```shell
sudo apt-get install libjemalloc-dev \
    libboost-dev \
    libboost-filesystem-dev \
    libboost-system-dev \
    libboost-regex-dev \
    python-dev \
    autoconf \
    flex \
    bison \
    libarrow-python-dev
    
python3.9 -m venv venv-from-source
source venv-from-source/bin/activate
pip install pip --upgrade
pip install wheel
pip install --no-binary pyarrow -r requirements-dev.txt
```

## Quick test

### Using setuptools

```shell
rm -rf *.so build/ && python setup.py build_ext --inplace &&  PYTHONPATH=./ pytest tests/
```

### Using CMake (deprecated)

```shell
cmake --build /home/arthur/source/cassarrow/cmake-build-debug --target _cassrrow -- -j 3 && PYTHONPATH=./:cmake-build-debug/cpp/ pytest tests
```

### Test library published in test pypi

```shell
python3.9 -m venv venv-test
source venv-test/bin/activate
pip install pip --upgrade
pip install --no-binary pyarrow pyarrow
pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/  cassarrow
python scripts/integration_test.py
```

### Get test data for local development

```shell
python scripts/dump_test_data.py 
```

## Resources

* https://docs.docker.com/engine/install/linux-postinstall/
* https://cassandra.apache.org/_/quickstart.html
* https://pybind11.readthedocs.io/en/stable/compiling.html

## TODO

* make wheels
* benchmark against numpy
* automate benchmark
* Read https://realpython.com/pypi-publish-python-package/
* Publish to pypi for tests
* add C++ tests
* Add reverse code (to inject from cassandra)
* Support for Map
* Support for Tuple?
* Support for Decimal
* Support time_uuid
* Support counter 
* Test nested UDT
* Test list of UDT
* Infer the schema from C++ code instead of python?

## Done

* More duration tests
* add UDT
* add with injector
* add list
* Save files from cassandra in docker
* Check the values make sense
* Supports for Set
* Move the code out of innit
* * Automate dump of binary files