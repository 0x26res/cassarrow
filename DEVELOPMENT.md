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



```

## Initial Set up

```shell
docker run --rm \
  --network cassandra \
  --volume "$(pwd)/migration.cql:/scripts/data.cql" \
  --env CQLSH_HOST=cassandra \
  --env CQLSH_PORT=9042 \
  --env CQLVERSION=3.4.5 \
  nuvo/docker-cqlsh

python ./scripts/populate_db.py 
python ./scripts/dump_test_data.py 
```

## Arrow Installation from source

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
rm -rf *.so build/ && USE_CXX11_ABI=0 python setup.py build_ext --inplace &&  PYTHONPATH=./ pytest tests/
```

### Using CMake 

```shell
cmake --build /home/arthur/source/cassarrow/cmake-build-debug --target _cassarrow -- -j 3 && PYTHONPATH=./:cmake-build-debug/cpp/ pytest tests
cmake --build /home/arthur/source/cassarrow/cmake-build-debug --target test_exe && ./cmake-build-debug/cpp/tests/test_exe
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

## Dicker
```shell
docker build --tag cassarrow:beta  .
docker run -it cassarrow:beta /bin/bash
```

## Version

```shell
bumpversion --current-version 0.1.0 patch setup.py cassarrow/__init__.py  --allow-dirty
```

## Wheel

Build locally:
```shell
USE_CXX11_ABI=0 python setup.py sdist bdist_wheel
tar tzf dist/cassarrow-0.0.0.tar.gz 
unzip -l dist/cassarrow-0.0.0-cp39-cp39-linux_x86_64.whl 
```

Check sdist:
```shell
export USE_CXX11_ABI=0
deactivate
python3.9 -m venv test-venv
source test-venv/bin/activate
pip install ./dist/cassarrow-0.0.0.tar.gz 
python -c "import _cassarrow; import pyarrow as pa; print(_cassarrow.parse_results(b'\0\0\0\1', pa.schema([])))"

```

## TODO

* run from docker
* make wheels + automate
    * Add tests to wheels
    * Add twine upload
    * Create pypi page
    * Read https://packaging.python.org/en/latest/
* benchmark against numpy
* Read https://realpython.com/pypi-publish-python-package/
* add C++ tests
* test all map values
* generate random data for tests?
* Support for Tuple?
* Support for Decimal
* Support counter 
* Test nested UDT
* Test list of UDT
* Infer the schema from C++ code instead of python?
* github action: run casandra db (or split unit and integration tests)

## Done

* More duration tests
* add UDT
* add with injector
* add list
* Save files from cassandra in docker
* Check the values make sense
* Supports for Set
* Move the code out of innit
* Automate dump of binary files
* automate benchmark
* Support time_uuid
* Support for Map

## Won't do

* Add reverse code (to inject from cassandra) -> no possible without ad hoc code (ask how it can be done?)