FROM gcc:latest

COPY . /usr/src/byor

WORKDIR /usr/src/byor

RUN g++ -o client client.cpp
RUN g++ -o server server.cpp

