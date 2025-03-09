#include "client.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
/**
 * This method assumes no pipelining
 */

std::vector<std::vector<std::string>> generate_cmds(size_t num_cmds)
{
    std::vector<std::vector<std::string>> cmds(num_cmds);

    for ( int shift = 0; shift < num_cmds; shift += 3 )
        cmds[shift] = { "set", std::string(32 << shift, '*'), std::string(32 << shift, '*') };

    return cmds;
}

void gen_plot_point(SocketClient<TcpTransport, RedisSerializer, RedisDeserializer>& client,
                    std::vector<double>& latencies, std::vector<std::string>& cmd)
{
    for ( size_t i = 0; i < latencies.size(); i++ )
    {
        // Start time
        auto start_time = std::chrono::high_resolution_clock::now();

        client.send_message(cmd);
        client.receive_message();

        auto end_time = std::chrono::high_resolution_clock::now();
        latencies[i] = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }
}

void calc_output(std::vector<double>& latencies)
{
    // End time
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double avg = sum / latencies.size();

    double min_latency = *std::min_element(latencies.begin(), latencies.end());
    double max_latency = *std::max_element(latencies.begin(), latencies.end());

    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[static_cast<size_t>(0.50 * latencies.size())];
    double p95 = latencies[static_cast<size_t>(0.95 * latencies.size())];
    double p99 = latencies[static_cast<size_t>(0.99 * latencies.size())];

    std::cout << "\nLatency Metrics (milliseconds):\n";
    std::cout << " Average Latency : " << avg << " ms\n";
    std::cout << " Minimum Latency : " << min_latency << " ms\n";
    std::cout << " Maximum Latency : " << max_latency << " ms\n";

    std::cout << "Median Latency  : " << p50 << " ms\n";
    std::cout << "95th Percentile : " << p95 << " ms\n";
    std::cout << "99th Percentile : " << p99 << " ms\n";

    // TODO: call export_data function to output to file to generate graphs
}

void run_benchmark(SocketClient<TcpTransport, RedisSerializer, RedisDeserializer>& client, RedisSerializer& serializer,
                   size_t num_req, size_t num_iter)
{
    std::vector<double> latencies(num_req);

    // Prepare request
    // std::vector<std::string> cmd{ "set", "key1", "val1" };
    auto cmds = generate_cmds(num_iter);

    for ( auto& cmd : cmds )
    {
        gen_plot_point(client, latencies, cmd);
        calc_output(latencies);
    }
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

    client.connect(addr, port); // TODO: probably should return bool if connection was successful

    constexpr size_t num_requests{ 1000 }; // Num of latencies to be averaged for one plot point
    constexpr size_t num_iterations{ 16 }; // Num of plot points to see how server scales with a larger request

    run_benchmark(client, serializer, num_requests, num_iterations);

    return 0;
}