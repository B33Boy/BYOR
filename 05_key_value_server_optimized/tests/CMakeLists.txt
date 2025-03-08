cmake_minimum_required(VERSION 3.14)
project(Tests)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(tests
    server_test.cpp
)

target_link_libraries(tests
    PRIVATE
    gtest
    gtest_main
    gmock 
    gmock_main
    spdlog::spdlog
)

# Register tests
include(GoogleTest)
gtest_discover_tests(tests)
