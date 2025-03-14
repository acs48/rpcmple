cmake_minimum_required(VERSION 3.16)
project(rpcmple)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(spdlog-1.14.1)

add_executable(rpcmple_cpp_example1 src_examples/example1.cpp)
target_include_directories(rpcmple_cpp_example1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpcmple_cpp_example1 spdlog_lib)

add_executable(rpcmple_cpp_example2 src_examples/example2.cpp)
target_include_directories(rpcmple_cpp_example2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpcmple_cpp_example2 spdlog_lib)

add_executable(rpcmple_cpp_example3 src_examples/example3.cpp)
target_include_directories(rpcmple_cpp_example3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpcmple_cpp_example3 spdlog_lib)

add_executable(rpcmple_cpp_example4socketserver src_examples/example4socketserver.cpp)
target_include_directories(rpcmple_cpp_example4socketserver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpcmple_cpp_example4socketserver spdlog_lib)

add_executable(rpcmple_cpp_example4socketclient src_examples/example4socketclient.cpp)
target_include_directories(rpcmple_cpp_example4socketclient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpcmple_cpp_example4socketclient spdlog_lib)

add_executable(rpcmple_cpp_example5Publisher src_examples/example5PublisherUDP.cpp)
target_include_directories(rpcmple_cpp_example5Publisher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpcmple_cpp_example5Publisher spdlog_lib)

add_executable(rpcmple_cpp_example5Subscriber src_examples/example5SubscriberUDP.cpp)
target_include_directories(rpcmple_cpp_example5Subscriber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpcmple_cpp_example5Subscriber spdlog_lib)

if (NOT TARGET rpcmple_lib)
    add_library(rpcmple_lib INTERFACE)
    target_include_directories(rpcmple_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(rpcmple_lib INTERFACE spdlog_lib)
endif()
