project(geo)
cmake_minimum_required(VERSION 3.0)

find_package(Boost)

include_directories(include)
include_directories(${Boost_INCLUDE_DIRS})

file(GLOB_RECURSE geo-files src/*.cc)
add_library(geo STATIC ${geo-files})
target_include_directories(geo PUBLIC include)
if (NOT MSVC)
  set_target_properties(geo PROPERTIES COMPILE_FLAGS "-std=c++14 -Wall -Wextra")
endif()
target_link_libraries(geo ${Boost_LIBRARIES})

file(GLOB_RECURSE geo-test-files test/*.cc)

add_executable(geo-test ${geo-test-files})
if (NOT MSVC)
  set_target_properties(geo-test PROPERTIES COMPILE_FLAGS "-std=c++14 -Wall -Wextra")
endif()
target_link_libraries(geo-test geo ${Boost_LIBRARIES})
