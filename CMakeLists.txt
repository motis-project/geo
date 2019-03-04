project(geo)
cmake_minimum_required(VERSION 3.0)

include_directories(include)

file(GLOB_RECURSE geo-files src/*.cc)
add_library(geo STATIC ${geo-files})
target_include_directories(geo INTERFACE include)
if (NOT MSVC)
  set_target_properties(geo PROPERTIES COMPILE_FLAGS "-std=c++14 -Wall -Wextra")
endif()

file(GLOB_RECURSE geo-test-files test/*.cc)

add_executable(geo-test ${geo-test-files})
if (NOT MSVC)
  set_target_properties(geo-test PROPERTIES COMPILE_FLAGS "-std=c++14 -Wall -Wextra")
endif()
target_link_libraries(geo-test geo)
