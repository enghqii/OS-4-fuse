cmake_minimum_required(VERSION 2.8.4)
project(FUSE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES hello.c)
add_executable(FUSE ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(FUSE fuse)
add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-DFUSE_USE_VERSION=22)