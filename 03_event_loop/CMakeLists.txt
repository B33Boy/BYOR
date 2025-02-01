cmake_minimum_required(VERSION 3.10)

# Set the project name
project(server)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the include directories
include_directories(include)

# Add the executable
add_executable(server src/main.cpp src/server.cpp src/epoll_wrapper.cpp)