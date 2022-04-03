# Wheels

WIP...

## Scope


The scope should be limited to wheels available for pyarrow (see https://pypi.org/project/pyarrow/#files):
- [ ] cp39-cp39-win_amd64.whl
- [x] cp39-cp39-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
- [ ] cp39-cp39-manylinux_2_17_aarch64.manylinux2014_aarch64.whl
- [ ] cp39-cp39-manylinux_2_12_x86_64.manylinux2010_x86_64.whl
- [ ] cp39-cp39-macosx_11_0_arm64.whl
- [ ] cp39-cp39-macosx_10_13_x86_64.whl
- [ ] cp39-cp39-macosx_10_13_universal2.whl
- [ ] cp39-cp39-macosx_10_9_x86_64.whl


The CI scipt has been borrowed from [zero mq](https://github.com/zeromq/pyzmq/blob/f30a5cb00cfd67ab65b474c86852a31e77c349af/.github/workflows/wheels.yml)


## Commands

### Build with no check

```shell
CIBW_BUILD="cp39*" CIBW_REPAIR_WHEEL_COMMAND='' cibuildwheel --platform=linux
```

### See what is being built

```shell
CIBW_BUILD="cp39*" CIBW_ARCHS_LINUX="x86_64"  cibuildwheel  --platform=linux --print-build-identifiers
```

## Debugging tools

### Run the wheel build manually

* Start the image
```shell
docker run -it --volume $(pwd):/project --platform=linux/386  quay.io/pypa/manylinux2014_i686:2021-12-12-e5100b5 /bin/bash
```
* In the image
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

rm -rf /tmp/cibuildwheel/repaired_wheel
mkdir -p /tmp/cibuildwheel/repaired_wheel

sh -c 'auditwheel repair -w /tmp/cibuildwheel/repaired_wheel /tmp/cibuildwheel/built_wheel/cassarrow-0.0.1rc0-cp39-cp39-linux_x86_64.whl'
```
* You may need to install arrow
```shell
yum install -y epel-release || yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1).noarch.rpm
yum install -y https://apache.jfrog.io/artifactory/arrow/centos/$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1)/apache-arrow-release-latest.rpm
yum install -y --enablerepo=epel arrow-devel # For C++
yum install -y --enablerepo=epel arrow-glib-devel # For GLib (C)
yum install -y --enablerepo=epel arrow-dataset-devel # For Apache Arrow Dataset C++
yum install -y --enablerepo=epel parquet-devel # For Apache Parquet C++
yum install -y --enablerepo=epel parquet-glib-devel # For Apache Parquet GLib (C)
yum install -y --enablerepo=epel arrow-python-devel # For Apache Parquet GLib (C)
```
* Or better, just pyarrow
```shell
pip install pyarrow
```

### Ways to fix the audithweel

```shell
ValueError: Cannot repair wheel, because required library "libarrow.so.700" could not be located

```

* Install the whole of arrow (slow/inefficient)
```shell
export CIBW_BEFORE_ALL="yum install -y epel-release || yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm && yum install -y https://apache.jfrog.io/artifactory/arrow/centos/7/apache-arrow-release-latest.rpm && yum install -y --enablerepo=epel arrow-python-devel"
CIBW_BUILD="cp39*" CIBW_ARCHS_LINUX="x86_64"  cibuildwheel  --platform=linux
```
* Install pyarrow and add it to the library path
```shell
export CIBW_REPAIR_WHEEL_COMMAND='pip install pyarrow && python -c "import pyarrow; pyarrow.create_library_symlinks()" && export LD_LIBRARY_PATH=/opt/_internal/cpython-3.9.9/lib/python3.9/site-packages/pyarrow/:$LD_LIBRARY_PATH && auditwheel repair -w {dest_dir} {wheel}'
```

## Test docker images to understand link errors

### How to build

```shell
docker build --tag=not-working    --file=not-working.Dockerfile .
docker build --tag=working        --file=not-working.Dockerfile .
docker build --tag=from-source    --file=from-source.Dockerfile .
docker build --tag=same-toolchain --file=same-toolchain.Dockerfile .
```

### How to run

```shell
docker run  from-source  python -c "import _cassarrow; import pyarrow as pa; print(_cassarrow.parse_results(b'\0\0\0\1', pa.schema([])))"
```
