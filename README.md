# Build Your Own Redis

Currently only working on Linux as it utilizes epoll().

The project is done in increments:
01_basic_server_client - basic tcp server & client
02_protocol_parsing - echo server with a length-prefixed protocol
03_event_loop - event-based concurrency leveraging epoll
04_key_value_server - high speed server with get/set/del commands implemented

## Building
```
cd 04_key_value_server/
mkdir build
cd build 
cmake ..
cmake --build .
```

## Running
#### Run the server and client in separate terminals
```
./server
```

```
./tempclient set key1 val1
./tempclient get key1
./tempclient del key1
```

