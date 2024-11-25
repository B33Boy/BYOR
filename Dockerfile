FROM gcc:latest

COPY . /usr/src/byor
WORKDIR /usr/src/byor

RUN /usr/src/byor/build.sh

