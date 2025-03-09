cmake_minimum_required(VERSION 3.14)
project(Benchmarks)

add_executable(throughput throughput.cpp)

target_link_libraries(throughput
    PRIVATE
    spdlog::spdlog
)

add_executable(latency latency.cpp)
target_link_libraries(latency
    PRIVATE
    spdlog::spdlog
)