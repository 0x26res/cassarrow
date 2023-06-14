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

## Poetry Set Up

```shell
poetry install
poetry run python -c "import cassarrow;print(cassarrow.__version__)"
poetry run pytest
```

## CMake set up 

```shell
mkdir -p ./cmake-build-debug/
(cd ./cmake-build-debug/ && cmake -Dpybind11_DIR=$(python -c "import pybind11;print(pybind11.get_cmake_dir())") -Dpyarrow_INCLUDE_DIR=$(python -c "import pyarrow;print(pyarrow.get_include())") -Dpyarrow_LIBRARIES=$(python -c "import pyarrow;print(' '.join(pyarrow.get_libraries()))") -Dpyarrow_LIBRARY_DIRS=$(python -c "import pyarrow;print(' '.join(pyarrow.get_library_dirs()))")  ../)
cmake --build ./cmake-build-debug --target _cassarrow -- -j 3 && PYTHONPATH=./:./cmake-build-debug/cpp/ pytest tests
cmake --build ./cmake-build-debug --target test_exe && ./cmake-build-debug/cpp/tests/test_exe
```

## Test library published in test pypi

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

## Docker

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

Or
```shell
python -m build
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

```shell
python -m twine upload --verbose --repository testpypi dist/
pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ cassarrow==0.1.2
```

## TODO

* Why does it throw an error and need the LD_LIBRARY_PATH?
* Find a license
* Upload a first version with wheels
* run from docker
* make wheels + automate
    * Add tests to wheels
    * Add twine upload
    * Create pypi page
    * Read https://packaging.python.org/en/latest/
* benchmark against numpy
* add C++ tests
* test all map values and keys
* generate random data for tests?
* Support for Tuple?
* Support for Decimal
* Support counter 
* Test nested UDT
* Test list of UDT
* Infer the schema from C++ code instead of python?
* github action: run casandra db (or split unit and integration tests)
* Wheels focus:
  - [x] cp39-cp39-macosx_10_9_x86_64 (untesed)
  - [x] cp310-cp310-macosx_10_9_x86_64 (untested)
  - [x] cp39-cp39-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
  - [x] cp310-cp310-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
  - [ ] aarch64 -> Can't install arrow? Does it need to?
  - [ ] windows
  - [ ] simpler example: https://github.com/PyTables/PyTables/blob/master/.github/workflows/wheels.yml
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

# Aarch64 wheels issue

```
python -m pip wheel /project --wheel-dir=/tmp/cibuildwheel/built_wheel --no-deps
command: /opt/python/cp39-cp39/bin/python /tmp/pip-standalone-pip-2_ppf9jw/__env_pip__.zip/pip install --ignore-installed --no-user --prefix /tmp/pip-build-env-yutkmtpz/overlay --no-warn-script-location --no-binary :none: --only-binary :none: -i https://pypi.org/simple -- 'setuptools>=42' wheel 'pybind11>=2.9.0' 'pyarrow>=7.0.0'
```

# Set up on Apple m1

```
intel
/usr/local/opt/python@3.10/bin/python3 -m venv py310 --clear
source py310/bin/activate
pip install --upgrade pip setuptools wheel
pip install --requirement requirements-dev.txt
export LD_LIBRARY_PATH=$(python -c 'import pyarrow; print(pyarrow.get_library_dirs()[0])')
rm -rf *.so build/ && USE_CXX11_ABI=0 python setup.py build_ext --inplace &&  PYTHONPATH=./ pytest tests/
```