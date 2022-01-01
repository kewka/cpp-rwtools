FROM ubuntu:16.04

WORKDIR /usr/src/rwtools

COPY src ./src
COPY include ./include
COPY Makefile .
RUN apt-get update && apt-get install make build-essential libpng-dev -y
RUN make

ENV PATH="/usr/src/rwtools/bin:${PATH}"