# Tries to install pyarrow from source and build the library
FROM python:3.9

# Environment
ENV container docker

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

RUN pip install --progress-bar=off --no-binary=pyarrow pyarrow
RUN pip install --progress-bar=off ./cassarrow/

# Cleaning
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && rm -rf ./cassarrow/
