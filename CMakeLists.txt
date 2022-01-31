cmake_minimum_required(VERSION 3.16.3)
project(cassarow)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -g -std=c++14 -fPIC")

add_subdirectory("cpp")
