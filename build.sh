#!/bin/bash
mkdir 01_basic_server_client/build
g++ -o 01_basic_server_client/build/myclient $(pwd)/01_basic_server_client/myclient.cpp
g++ -o 01_basic_server_client/build/myserver $(pwd)/01_basic_server_client/myserver.cpp

