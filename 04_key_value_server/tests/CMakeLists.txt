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
    epollwrapper_test.cpp
)

# Link GoogleTest and your project's library
target_link_libraries(tests
    PRIVATE
    gtest
    gtest_main
    gmock 
    gmock_main
)

# Register tests
include(GoogleTest)
gtest_discover_tests(tests)
