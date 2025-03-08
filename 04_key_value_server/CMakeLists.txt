cmake_minimum_required(VERSION 3.10)

project(server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
    set(SANITIZER_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

add_library(server_lib 
    STATIC 
    src/server.cpp 
    src/epoll_wrapper.cpp 
    src/socketwrapper.cpp)

add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE server_lib)

add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE server_lib)

add_executable(cli_client src/cli_client.cpp)

add_subdirectory(tests)