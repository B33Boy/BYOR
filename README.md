# Build Your Own Redis

Currently only working on Linux as it utilizes epoll().

The project is done in increments:
01_basic_server_client - basic tcp server & client
02_protocol_parsing - echo server with a length-prefixed protocol
03_event_loop - event-based concurrency leveraging epoll
04_key_value_server - high speed server with get/set/del commands implemented
05_key_value_server_optimized - server optimized for low latency and high throughput

## Building
```
cd 05_key_value_server_optimized/
mkdir build
cd build 
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

## Running
#### Run the server and client in separate terminals
```
./server
```

```
./client 127.0.0.1 1234 set key1 val1
./client 127.0.0.1 1234 get key1
./client 127.0.0.1 1234 del key1
```

## Benchmarking
Each benchmark is a different executable located at `05_key_value_server_optimized/build/benchmarks/`
- `./latency 127.0.0.1 1234`
- `./throughput 127.0.0.1 1234`

