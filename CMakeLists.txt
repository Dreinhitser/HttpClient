cmake_minimum_required(VERSION 3.20)
project(http_client)

file(GLOB SRC *.cpp)

add_executable(http_client ${SRC})

set_property(TARGET http_client PROPERTY CXX_STANDARD 17)
