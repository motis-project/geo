project(geo)
cmake_minimum_required(VERSION 3.0)

include_directories(include)

file(GLOB_RECURSE geo-files src/*.cc)
add_library(geo STATIC ${geo-files})
set_target_properties(geo PROPERTIES COMPILE_FLAGS "-std=c++14 -Wall -Wextra")

file(GLOB_RECURSE geo-test-files test/*.cc)

add_executable(geo-test ${geo-test-files})
set_target_properties(geo-test PROPERTIES COMPILE_FLAGS "-std=c++14 -Wall -Wextra")
target_link_libraries(geo-test geo)
