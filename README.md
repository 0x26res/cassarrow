# Cassarrow
Arrow based Cassandra python driver

## TODO

* Check the value make sense
* test every possible types https://github.com/datastax/cpp-driver/blob/151f3988e60434a740a9946c030c17ed6c9a7b9e/src/decoder.hpp#L495
* add C++ tests
* Add reverse code (to inject from cassandra)
* Add type mapping documentation

## Done

* add UDT
* add with injector
* add list
* Save files from cassandra in docker 

## Cassandra set up

Follow this guide https://cassandra.apache.org/_/quickstart.html

Helper commands:

```shell
docker pull cassandra
docker network create cassandra

docker run -rm --name cassandra -p 127.0.0.1:9042:9042 
docker run --rm --detach \
  --name cassandra \
  --hostname cassandra \
  --network cassandra \
  --publish 127.0.0.1:9042:9042 \
  --publish 127.0.0.1:9160:9160 \
  --volume /home/arthur/data/cassandra:/var/lib/cassandra \
  cassandra

docker run -rm --name cassandra -p 127.0.0.1:9042:9042 -p 127.0.0.1:9160:9160 -d cassandra 
docker run --name cassandra -p 127.0.0.1:9042:9042 -p 127.0.0.1:9160:9160 -d cassandra 
docker run --rm -it --network cassandra nuvo/docker-cqlsh cqlsh cassandra 9042 --cqlversion='3.4.5'
docker run --rm -it --network cassandra nuvo/docker-cqlsh cqlsh cassandra 9042 --cqlversion='3.4.5'

docker logs --follow cassandra

docker run --rm \
  --network cassandra \
  --volume "$(pwd)/migration.cql:/scripts/data.cql" \
  --env CQLSH_HOST=cassandra \
  --env CQLSH_PORT=9042 \
  --env CQLVERSION=3.4.5 \
  nuvo/docker-cqlsh
  
du -sh /home/arthur/data/cassandra

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
cmake --build /home/arthur/source/cassarrow/cmake-build-debug --target bindings -- -j 3 && PYTHONPATH=./:cmake-build-debug/cpp/ pytest tests
```

## Build with setuptools

```shell
python setup.py build_ext --inplace
```

## Resources


* https://docs.docker.com/engine/install/linux-postinstall/
* https://cassandra.apache.org/_/quickstart.html
  