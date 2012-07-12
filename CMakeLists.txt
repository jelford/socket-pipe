cmake_minimum_required (VERSION 2.6)
project (EchoServer)

set (EchoServer_VERSION_MAJOR 1)
set (EchoServer_VERSION_MINOR 0)

# Set the compiler to use C++11
add_definitions("-std=c++11")

# Include the Sockets submodule
include_directories ("${PROJECT_SOURCE_DIR}"Sockets/include/)
add_subdirectory (Sockets)

add_executable (EchoServer main.cpp)
target_link_libraries (EchoServer Sockets)
