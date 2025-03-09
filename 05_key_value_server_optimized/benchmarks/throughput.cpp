#include "client.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

void run_benchmark(SocketClient<TcpTransport, RedisSerializer, RedisDeserializer>& client, size_t num_requests,
                   RedisSerializer& serializer)
{
    size_t req_sent{ 0 };
    size_t resp_recv{ 0 };

    // Prepare num_requests
    std::vector<std::string> cmd{ "set", "key1", "val1" };
    auto const req = serializer.serialize(cmd);

    auto send_req_log = [&client, &req_sent](std::vector<std::string>& message)
    {
        client.send_message(message);
        req_sent++;
    };

    auto resp_recv_log = [&client, &resp_recv]()
    {
        client.receive_message();
        resp_recv++;
    };

    // Start time
    auto start_time = std::chrono::high_resolution_clock::now();

    for ( size_t i = 0; i < num_requests; i++ )
    {
        send_req_log(cmd);
        resp_recv_log();
    }

    // End time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed{ std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time) };

    double rps = num_requests / elapsed.count();

    std::cout << "Requests Sent: " << req_sent << "\n";
    std::cout << "Responses Received: " << resp_recv << "\n";
    std::cout << "Time Elapsed: " << elapsed.count() << " seconds\n";
    std::cout << "Requests Per (RPS): " << rps << "\n";
}

int main(int argc, char* argv[])
{
    if ( argc < 3 )
    {
        std::cout << "Input needs to be of the form: ./req_per_sec SERVER PORT";
        return 1;
    }

    TcpTransport transport;
    RedisSerializer serializer;
    RedisDeserializer deserializer;

    SocketClient<TcpTransport, RedisSerializer, RedisDeserializer> client(transport, serializer, deserializer);

    std::string const addr = argv[1];
    size_t const port = std::stoi(argv[2]);

    client.connect(addr, port); // probably should return bool if connection was successful

    size_t num_requests{ 10000 };

    run_benchmark(client, num_requests, serializer);

    return 0;
}