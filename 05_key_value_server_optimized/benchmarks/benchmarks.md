# Results

## Server with Virtual Dispatch (04_key_value_server)
#### Req Per Second (Debug, with print statements)
    Requests Sent: 10000
    Responses Received: 10000
    Time Elapsed: 56 seconds
    Requests Per  (RPS): 178

#### Req Per Second (Release)
    Requests Sent: 10000
    Responses Received: 10000
    Time Elapsed: 39 seconds
    Requests Per  (RPS): 256


## CRTP Server (05_key_value_server_optimized)
#### Req Per Second (Debug, with print statements)
    Requests Sent: 10000
    Responses Received: 10000
    Time Elapsed: 40 seconds
    Requests Per (RPS): 250

#### Req Per Second (Release)
    Requests Sent: 10000
    Responses Received: 10000
    Time Elapsed: 36 seconds
    Requests Per (RPS): 277


Conclusion: the performance gain with CRTP is minimal?