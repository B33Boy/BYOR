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

## Benchmarks
Must have google-benchmark built and installed (ideally globally)
Confirm installation at `/usr/local/include/benchmark`

#### Metrics
- Throughput in requests/sec
- Latency in ms/request
- CPU Usage
- How the above metrics scale with multiple clients

#### Micro-Benchmarks
`void parse_req(uint8_t const* data, size_t size, std::vector<std::string>& parsed_cmds)`
- Measure how speed scales with data of varying size

`void do_request(std::vector<std::string> const& cmd, Response& resp)`
- Measure how fast each command is (set, get, del, etc)

`void make_response(Response& resp, std::vector<uint8_t>& out)`
- Measure how fast a response is populated from each outgoing buffer

`handle_new_connections() noexcept;`
`handle_read_event(Connection& conn);`
`handle_write_event(Connection& conn);`



