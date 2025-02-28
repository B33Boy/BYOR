#include "client.h"
#include "server.h"

#include <benchmark/benchmark.h>

// Before running benchmarks
void SetUp()
{
    // // Start server process separately
    // server_pid = StartServerAsExternalProcess();
}

static void BM_ServerEndToEnd(benchmark::State& state)
{
    // Create client socket and connect to server

    for ( auto _ : state )
    {
        // send_req()
        // receive_res()
        benchmark::DoNotOptimize(buffer);
    }

    close(client_sock);
}

// After all benchmarks
void TearDown()
{
    // // Terminate server process
    // StopExternalServer(server_pid);
}

BENCHMARK(BM_ServerEndToEnd);

BENCHMARK_MAIN();