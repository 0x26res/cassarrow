#
# https://github.com/cpp-projects-showcase/docker-images/tree/master/ubuntu2004
#
#FROM docker.io/ubuntu:20.04
FROM python:3.9
MAINTAINER Denis Arnaud <denis.arnaud_github at m4x dot org>
LABEL version="0.1"

# Environment
ENV container docker
ENV HOME /home/build
ENV LANGUAGE en_US:en
ENV LANG en_US.UTF-8
ENV LC_ALL $LANG


# Update the system
RUN echo 'APT::Get::Assume-Yes "true";' > /etc/apt/apt.conf.d/90-yes
RUN apt-get -qq update
# RUN apt-get -y install python3.9 python3.9-dev python3.9-venv

RUN apt-get update && \
    apt-get install -y -V ca-certificates lsb-release wget && \
    wget "https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb" && \
    apt-get install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt-get update && \
    apt-get install -y -V libarrow-dev && \
    apt-get install -y -V libarrow-glib-dev && \
    apt-get install -y -V libarrow-dataset-dev && \
    apt-get install -y -V libarrow-flight-dev && \
    apt-get install -y -V libparquet-dev && \
    apt-get install -y -V libarrow-python-dev

RUN rm apache-arrow-apt-source-latest-bullseye.deb

RUN apt-get install -y cmake

ADD ./ ./cassarrow

RUN pip install --progress-bar=off --requirement=./cassarrow/requirements.txt

RUN pip install --progress-bar=off ./cassarrow/

# Cleaning
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && rm -rf ./cassarrow/
