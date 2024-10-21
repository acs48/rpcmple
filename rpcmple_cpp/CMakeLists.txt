cmake_minimum_required(VERSION 3.15)
project(rpcmple)

set(CMAKE_CXX_STANDARD 17)

add_executable(rpcmple_example src_examples/example1.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
