#include "client.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
/**
 * This method assumes no pipelining
 */
void run_benchmark(SocketClient<TcpTransport, RedisSerializer, RedisDeserializer>& client, size_t num_requests,
                   RedisSerializer& serializer)
{
    std::vector<double> latencies{};

    // Prepare request
    std::vector<std::string> cmd{ "set", "key1", "val1" };

    for ( size_t i = 0; i < num_requests; i++ )
    {
        // Start time
        auto start_time = std::chrono::high_resolution_clock::now();

        client.send_message(cmd);
        client.receive_message();

        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        latencies.push_back(elapsed);
    }

    // End time
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double avg = sum / latencies.size();

    double min_latency = *std::min_element(latencies.begin(), latencies.end());
    double max_latency = *std::max_element(latencies.begin(), latencies.end());

    std::cout << "\nLatency Metrics (milliseconds):\n";
    std::cout << " Average Latency : " << avg << " ms\n";
    std::cout << " Minimum Latency : " << min_latency << " ms\n";
    std::cout << " Maximum Latency : " << max_latency << " ms\n";

    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[static_cast<size_t>(0.50 * latencies.size())];
    double p95 = latencies[static_cast<size_t>(0.95 * latencies.size())];
    double p99 = latencies[static_cast<size_t>(0.99 * latencies.size())];

    std::cout << "Median Latency  : " << p50 << " ms\n";
    std::cout << "95th Percentile : " << p95 << " ms\n";
    std::cout << "99th Percentile : " << p99 << " ms\n";
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