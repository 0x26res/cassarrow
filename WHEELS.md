# Wheels

## Inspiration

Use [zero mq](https://github.com/zeromq/pyzmq/blob/f30a5cb00cfd67ab65b474c86852a31e77c349af/.github/workflows/wheels.yml) version:


## Commands

### Build not check

```shell
CIBW_BUILD="cp39*" CIBW_REPAIR_WHEEL_COMMAND='' cibuildwheel --platform=linux
```

```shell
CIBW_BUILD="cp39-manylinux_i686"cibuildwheel --platform=linux
```

Avoid this error on cp39-manylinux_i68:
```shell
# Error: Command ['python', '-m', 'pip', 'wheel', PurePosixPath('/project'), '--wheel-dir=/tmp/cibuildwheel/built_wheel', '--no-deps'] failed with code 1.
# ERROR: Could not build wheels for pyarrow, which is required to install pyproject.toml-based projects
# Could NOT find Arrow (missing: ARROW_INCLUDE_DIR ARROW_LIB_DIR
```

Expending:
```shell
CIBW_BUILD="cp39*" CIBW_ARCHS_LINUX="x86_64"  cibuildwheel  --platform=linux --print-build-identifiers 
CIBW_BUILD="cp39*" CIBW_ARCHS_LINUX="x86_64"  cibuildwheel  --platform=linux 

```


In image quay.io/pypa/manylinux2014_x86_64:2021-12-12-e5100b5
```shell

docker run -it --volume $(pwd):/project   quay.io/pypa/manylinux2014_x86_64:2021-12-12-e5100b5 -- /bin/bash

PIP_DISABLE_PIP_VERSION_CHECK="1"
mkdir -p /project

PATH=/opt/python/cp39-cp39/bin:$PATH
/opt/python/cp38-cp38/bin/python -c 'import sys, json, os; json.dump(os.environ.copy(), sys.stdout)'
which python
which pip

rm -rf /tmp/cibuildwheel/built_wheel
mkdir -p /tmp/cibuildwheel/built_wheel
python -m pip wheel /project --wheel-dir=/tmp/cibuildwheel/built_wheel --no-deps

/opt/python/cp38-cp38/bin/python -c 'import sys, json, glob; json.dump(glob.glob('"'"'/tmp/cibuildwheel/built_wheel/*.whl'"'"'), sys.stdout)'
rm -rf /tmp/cibuildwheel/repaired_wheel
mkdir -p /tmp/cibuildwheel/repaired_wheel



yum install -y epel-release || yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1).noarch.rpm
yum install -y https://apache.jfrog.io/artifactory/arrow/centos/$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1)/apache-arrow-release-latest.rpm
yum install -y --enablerepo=epel arrow-devel # For C++
yum install -y --enablerepo=epel arrow-glib-devel # For GLib (C)
yum install -y --enablerepo=epel arrow-dataset-devel # For Apache Arrow Dataset C++
yum install -y --enablerepo=epel parquet-devel # For Apache Parquet C++
yum install -y --enablerepo=epel parquet-glib-devel # For Apache Parquet GLib (C)
yum install -y --enablerepo=epel arrow-python-devel # For Apache Parquet GLib (C)

sh -c 'auditwheel repair -w /tmp/cibuildwheel/repaired_wheel /tmp/cibuildwheel/built_wheel/cassarrow-0.0.1rc0-cp39-cp39-linux_x86_64.whl'

```

```shell
export CIBW_BEFORE_ALL="yum install -y epel-release || yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm && yum install -y https://apache.jfrog.io/artifactory/arrow/centos/7/apache-arrow-release-latest.rpm && yum install -y --enablerepo=epel arrow-python-devel"
CIBW_BUILD="cp39*" CIBW_ARCHS_LINUX="x86_64"  cibuildwheel  --platform=linux
```

* [ ] TODO: check it works on quay.io/pypa/musllinux_1_1_x86_64:2021-12-12-e5100b5
```shell
CIBW_BEFORE_BUILD="pip install pyarrow"
CIBW_BUILD="cp39-musllinux_x86_64"  CIBW_ARCHS_LINUX="x86_64"  cibuildwheel  --platform=linux 

```
> Could NOT find Arrow (missing: ARROW_INCLUDE_DIR ARROW_LIB_DIR


```


Traceback (most recent call last):
File "/usr/local/bin/auditwheel", line 8, in <module>
sys.exit(main())
File "/opt/_internal/pipx/venvs/auditwheel/lib/python3.9/site-packages/auditwheel/main.py", line 59, in main
rval = args.func(args, p)
File "/opt/_internal/pipx/venvs/auditwheel/lib/python3.9/site-packages/auditwheel/main_repair.py", line 161, in execute
out_wheel = repair_wheel(
File "/opt/_internal/pipx/venvs/auditwheel/lib/python3.9/site-packages/auditwheel/repair.py", line 74, in repair_wheel
raise ValueError(
ValueError: Cannot repair wheel, because required library "libarrow.so.700" could not be located
```

```
CIBW_BUILD="cp38*" cibuildwheel --platform=linux
# auditwheel repair -w {dest_dir} {wheel}

#Error: Command ['sh', '-c', 'auditwheel repair -w /tmp/cibuildwheel/repaired_wheel /tmp/cibuildwheel/built_wheel/cassarrow-0.0.1rc0-cp39-cp39-linux_x86_64.whl'] failed with code 1.
# -> Use CIBW_REPAIR_WHEEL_COMMAND=''
```


docker run -it  quay.io/pypa/manylinux2014_i686:2021-12-12-e5100b5 /bin/bash

```shell


    -- Configuring incomplete, errors occurred!
    See also "/tmp/pip-install-swhfv25g/pyarrow_922e87f2a9294760a5ffa4cca762f26b/build/temp.linux-i686-3.9/CMakeFiles/CMakeOutput.log".
    error: command '/usr/local/bin/cmake' failed with exit code 1
    ----------------------------------------
    ERROR: Failed building wheel for pyarrow
    Building wheel for cassandra-driver (setup.py): started
    Building wheel for cassandra-driver (setup.py): still running...
    Building wheel for cassandra-driver (setup.py): still running...
    Building wheel for cassandra-driver (setup.py): still running...
    Building wheel for cassandra-driver (setup.py): finished with status 'done'
    Created wheel for cassandra-driver: filename=cassandra_driver-3.25.0-cp39-cp39-linux_i686.whl size=18569250 sha256=4dca3adade637f4f6bd36a21dffcd170d8690e2ba78c3969c403387e18566d6d
    Stored in directory: /root/.cache/pip/wheels/3d/bd/75/12875d7c70c5b18e8738c9e06f2d3f5b752fa372917c663fcc
    Building wheel for numpy (pyproject.toml): started
    Building wheel for numpy (pyproject.toml): still running...
    Building wheel for numpy (pyproject.toml): still running...
    Building wheel for numpy (pyproject.toml): finished with status 'done'
    Created wheel for numpy: filename=numpy-1.22.3-cp39-cp39-linux_i686.whl size=21885071 sha256=e27b48ee3daf691de6514f806b322bfad7d500b5de8def4c29a99fb95eb51cc9
    Stored in directory: /root/.cache/pip/wheels/1b/48/26/7dcbae6a5a0912a3486f7991673eba6fcd8bb87245a067b97f
Successfully built cassandra-driver numpy
Failed to build pyarrow
ERROR: Could not build wheels for pyarrow, which is required to install pyproject.toml-based projects
WARNING: You are using pip version 21.3.1; however, version 22.0.4 is available.
You should consider upgrading via the '/opt/python/cp39-cp39/bin/python -m pip install --upgrade pip' command.
  ----------------------------------------
WARNING: Discarding file:///project. Command errored out with exit status 1: /opt/python/cp39-cp39/bin/python /tmp/pip-standalone-pip-vtsc0u2i/__env_pip__.zip/pip install --ignore-installed --no-user --prefix /tmp/pip-build-env-98fwasl3/overlay --no-warn-script-location --no-binary :none: --only-binary :none: -i https://pypi.org/simple -- 'setuptools>=42' wheel 'pybind11>=2.9.0' 'pyarrow>=7.0.0' cassandra-driver Check the logs for full command output.
ERROR: Command errored out with exit status 1: /opt/python/cp39-cp39/bin/python /tmp/pip-standalone-pip-vtsc0u2i/__env_pip__.zip/pip install --ignore-installed --no-user --prefix /tmp/pip-build-env-98fwasl3/overlay --no-warn-script-location --no-binary :none: --only-binary :none: -i https://pypi.org/simple -- 'setuptools>=42' wheel 'pybind11>=2.9.0' 'pyarrow>=7.0.0' cassandra-driver Check the logs for full command output.
WARNING: You are using pip version 21.3.1; however, version 22.0.4 is available.
You should consider upgrading via the '/opt/python/cp39-cp39/bin/python -m pip install --upgrade pip' command.
Processing /project
Installing build dependencies: started
Installing build dependencies: still running...
Installing build dependencies: still running...
Installing build dependencies: still running...
Installing build dependencies: still running...
Installing build dependencies: still running...
Installing build dependencies: finished with status 'error'

                                                            ✕ 441.59s
Error: Command ['python', '-m', 'pip', 'wheel', PurePosixPath('/project'), '--wheel-dir=/tmp/cibuildwheel/built_wheel', '--no-deps'] failed with code 1.

Checking for common errors...

NOTE: Shared object (.so) files found in this project.

These files might be built against the wrong OS, causing problems with
auditwheel. If possible, run cibuildwheel in a clean checkout.

If you're using Cython and have previously done an in-place build,
remove those build files (*.so and *.c) before starting cibuildwheel.

setuptools uses the build/ folder to store its build cache. It
may be necessary to remove those build files (*.so and *.o) before
starting cibuildwheel.

Files that belong to a virtual environment are probably not an issue
unless you used a custom command telling cibuildwheel to activate it.

Files detected:
_cassarrow.cpython-39-x86_64-linux-gnu.so
build/lib.linux-x86_64-3.9/_cassarrow.cpython-39-x86_64-linux-gnu.so
cmake-build-debug/cpp/src/_cassarrow.cpython-39-x86_64-linux-gnu.so

```


# DOCKER BUILDS

```shell
docker build --tag=not-working    --file=not-working.Dockerfile .
docker build --tag=working    --file=not-working.Dockerfile .
docker build --tag=from-source    --file=from-source.Dockerfile .
docker build --tag=same-toolchain --file=same-toolchain.Dockerfile .

docker run  from-source  python -c "import _cassarrow; import pyarrow as pa; print(_cassarrow.parse_results(b'\0\0\0\1', pa.schema([])))"
docker run  not-working  python -c "import _cassarrow; import pyarrow as pa; print(_cassarrow.parse_results(b'\0\0\0\1', pa.schema([])))"
docker run  same-toolchain python -c "import _cassarrow; import pyarrow as pa; print(_cassarrow.parse_results(b'\0\0\0\1', pa.schema([])))"
 
```
Try this docker image: #FROM docker.io/ubuntu:20.04


