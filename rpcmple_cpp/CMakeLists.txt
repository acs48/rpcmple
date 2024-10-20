cmake_minimum_required(VERSION 3.29)
project(rpcmple_example)

set(CMAKE_CXX_STANDARD 17)

include_directories(rpcmple)

add_executable(rpcmple_example example1.cpp)
add_compile_definitions(BUFFER_SIZE_DEFINITION=10240)