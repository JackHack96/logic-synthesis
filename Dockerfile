FROM ubuntu:22.04

LABEL version="0.1"
LABEL description="Docker image for building old logic synthesis EDA tools on Ubuntu 18.04"

RUN apt-get update -y

# Install dependencies
RUN apt-get install -y make gcc bison flex build-essential libreadline-dev gcc-9

# Install development tools
RUN apt-get install -y git vim

RUN useradd -m dev
USER dev

# Map the project contents in.
ADD --chown=dev . /home/dev/logic-synthesis/
WORKDIR /home/dev/logic-synthesis/

RUN cd /home/dev/logic-synthesis/ && \
  CC=gcc-9 CFLAGS="-fno-stack-protector" ./configure && \
  make
