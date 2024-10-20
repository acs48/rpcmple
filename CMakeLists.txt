cmake_minimum_required(VERSION 3.15)
project(rpcmple VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(rpcmple INTERFACE)
target_include_directories(rpcmple INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rpcmple>"
        "$<INSTALL_INTERFACE:include>"
)
# Set version information
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/rpcmpleConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

# Install the library
install(DIRECTORY rpcmple_cpp/rpcmple/ DESTINATION rpcmple)
install(TARGETS rpcmple
        EXPORT rpcmpleTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(EXPORT rpcmpleTargets
        FILE rpcmpleTargets.cmake
        NAMESPACE rpcmple::
        DESTINATION lib/cmake/rpcmple)

# Export the package for use from the build tree
export(EXPORT rpcmpleTargets FILE "${CMAKE_CURRENT_BINARY_DIR}/rpcmpleTargets.cmake" NAMESPACE rpcmple::)

# Create a Config file for find_package
configure_file(rpcmpleConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/rpcmpleConfig.cmake" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rpcmpleConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/rpcmpleConfigVersion.cmake"
        DESTINATION lib/cmake/rpcmple)