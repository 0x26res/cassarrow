#
# https://github.com/cpp-projects-showcase/docker-images/tree/master/ubuntu2004
#
FROM docker.io/ubuntu:20.04
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
RUN apt -qq update
RUN apt -y install python3.9 python3.9-dev python3.9-venv

RUN apt update && \
    apt install -y -V ca-certificates lsb-release wget && \
    wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt update && \
    apt install -y -V libarrow-dev && \
    apt install -y -V libarrow-glib-dev && \
    apt install -y -V libarrow-dataset-dev && \
    apt install -y -V libarrow-flight-dev && \
    apt install -y -V libparquet-dev


# Cleaning
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Entry point
CMD ["/bin/bash"]

