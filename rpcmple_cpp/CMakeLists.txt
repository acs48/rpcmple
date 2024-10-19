cmake_minimum_required(VERSION 3.29)
project(01_6_RPCMPLE_CPP)

set(CMAKE_CXX_STANDARD 17)

include_directories(rpcmple)

add_executable(01_6_RPCMPLE_CPP
        example1.cpp)
add_compile_definitions(BUFFER_SIZE_DEFINITION=10240)