cmake_minimum_required(VERSION 3.16)
project(spdlog)

set(CMAKE_CXX_STANDARD 11)

if (NOT TARGET spdlog_lib)
    add_library(spdlog_lib INTERFACE)
    target_include_directories(spdlog_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
