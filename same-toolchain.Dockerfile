# Tries to install pyarrow from source and build the library
#FROM docker.io/ubuntu:20.04
FROM quay.io/pypa/manylinux2014_x86_64:2021-12-12-e5100b5

# Environment
ENV PATH=/opt/python/cp39-cp39/bin:$PATH

ADD ./ ./cassarrow


# RUN pip install --progress-bar=off cassandra-driver pandas pyarrow pybind11

RUN pip install --progress-bar=off pyarrow ./cassarrow/

# Not sure why I have to do that:
RUN python -c "import pyarrow;pyarrow.create_library_symlinks()"
ENV LD_LIBRARY_PATH=/opt/_internal/cpython-3.9.9/lib/python3.9/site-packages/pyarrow/:$LD_LIBRARY_PATH
# Cleaning
RUN rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && rm -rf ./cassarrow/
