cmake_minimum_required(VERSION 3.14)
project(Benchmarks)

add_executable(throughput throughput.cpp)


target_link_libraries(throughput
    PRIVATE
    spdlog::spdlog
)

# Add sanitizers if in Debug build
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SANITIZER_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()


add_executable(latency latency.cpp)
target_link_libraries(latency
    PRIVATE
    spdlog::spdlog
)