cmake_minimum_required(VERSION 3.15)
project(rpcmple)

set(CMAKE_CXX_STANDARD 17)

add_executable(rpcmple_example src_examples/example1.cpp)
target_include_directories(rpcmple_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(rpcmple_lib INTERFACE)
target_include_directories(rpcmple_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
