#!/bin/bash
mkdir -p 01_basic_server_client/build
g++ -o 01_basic_server_client/build/myclient $(pwd)/01_basic_server_client/myclient.cpp
g++ -o 01_basic_server_client/build/myserver $(pwd)/01_basic_server_client/myserver.cpp


mkdir -p 02_protocol_parsing/build
g++ -o 02_protocol_parsing/build/myclient $(pwd)/02_protocol_parsing/myclient.cpp
g++ -o 02_protocol_parsing/build/myserver $(pwd)/02_protocol_parsing/myserver.cpp
