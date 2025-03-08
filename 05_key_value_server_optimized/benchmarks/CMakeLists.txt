cmake_minimum_required(VERSION 3.14)
project(Benchmarks)

add_executable(req_per_sec req_per_sec.cpp)

target_link_libraries(req_per_sec
    PRIVATE
    spdlog::spdlog
)