cmake_minimum_required(VERSION 3.10)

project(server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")

if(CMAKE_BUILD_SANITIZE)
message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
set(SANITIZER_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

include_directories(include)

include(FetchContent)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.1
)
FetchContent_MakeAvailable(spdlog)

add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE spdlog::spdlog)

add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE spdlog::spdlog)

function(set_spdlog_level target)
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
        target_compile_definitions(${target} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG SPDLOG_LEVEL=SPDLOG_LEVEL_DEBUG)
    else()
        target_compile_definitions(${target} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN SPDLOG_LEVEL=SPDLOG_LEVEL_WARN )
        
    endif()
endfunction()

set_spdlog_level(server)
set_spdlog_level(client)

add_subdirectory(tests)
add_subdirectory(benchmarks)