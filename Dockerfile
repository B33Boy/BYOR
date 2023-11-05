FROM gcc:latest

COPY . /usr/src/byor
WORKDIR /usr/src/byor

RUN build.sh

