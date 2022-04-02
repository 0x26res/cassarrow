# Tries to install the library with the standard arrow wheel
FROM python:3.9

ENV container docker

ADD ./ ./cassarrow

ENV USE_CXX11_ABI=0

RUN pip install --progress-bar=off pyarrow ./cassarrow/

RUN python -c "import pyarrow;pyarrow.create_library_symlinks()"
ENV LD_LIBRARY_PATH=/usr/local/lib/python3.9/site-packages/pyarrow/:$LD_LIBRARY_PATH

# Cleaning
RUN rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && rm -rf ./cassarrow/