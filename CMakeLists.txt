cmake_minimum_required(VERSION 3.10)

project(TCPServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(server src/server.cpp)
