project(geo)
cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE geo-files src/*.cc)
add_library(geo STATIC ${geo-files})
set_target_properties(geo PROPERTIES COMPILE_FLAGS "-std=c++14 -Wall -Wextra")

