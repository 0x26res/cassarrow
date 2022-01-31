# Cassarrow
Arrow based Cassandra python driver

## TODO

"cassandra/row_parser.pyx": def recv_results_rows should call pybind and make the magic happen

## Cassandra set up

Follow this guide https://cassandra.apache.org/_/quickstart.html

Helper commands:

```shell
docker pull cassandra
docker network create cassandra
docker run --rm -d --name cassandra --hostname cassandra --network cassandra -p 127.0.0.1:9042:9042 -p 127.0.0.1:9160:9160 cassandra

docker run -rm --name cassandra -p 127.0.0.1:9042:9042 -p 127.0.0.1:9160:9160 -d cassandra 
docker run --rm -it --network cassandra nuvo/docker-cqlsh cqlsh cassandra 9042 --cqlversion='3.4.5'
docker run --rm -it --network cassandra nuvo/docker-cqlsh cqlsh cassandra 9042 --cqlversion='3.4.5'
```

## Installation

```shell
python3 -m venv venv
source venv/bin/activate
pip install pip --upgrade
pip install wheel
pip install -r requirements.txt
```

## Quick test

```shell
PYTHONPATH=cmake-build-debug/cpp/ python -c "import bindings; print(bindings.test_arrow())"

```

## Resources


https://docs.docker.com/engine/install/linux-postinstall/