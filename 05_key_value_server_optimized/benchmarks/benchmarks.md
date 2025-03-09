# Results

## Throughput
### 04_key_value_server 
    Requests Sent: 10000
    Responses Received: 10000
    Time Elapsed: 39 seconds
    Requests Per  (RPS): 256

### 05_key_value_server_optimized
    Requests Sent: 10000
    Responses Received: 10000
    Time Elapsed: 36 seconds
    Requests Per (RPS): 277


## Latency
### 04_key_value_server 
    Latency Metrics (milliseconds):
    Average Latency : 4.05327 ms
    Minimum Latency : 3.53736 ms
    Maximum Latency : 18.5291 ms
    Median Latency  : 3.87922 ms
    95th Percentile : 5.02765 ms
    99th Percentile : 6.50317 ms


### 05_key_value_server_optimized 
    Latency Metrics (milliseconds):
    Average Latency : 3.83761 ms
    Minimum Latency : 3.50916 ms
    Maximum Latency : 13.8758 ms
    Median Latency  : 3.6943 ms
    95th Percentile : 4.4623 ms
    99th Percentile : 5.61186 ms


Conclusion: the performance gain with CRTP is minimal?