cmake_minimum_required(VERSION 3.16)
project(geo)

find_package(Boost)

include_directories(include)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

include(cmake/pkg.cmake)
include(cmake/clang-tidy.cmake)

file(GLOB_RECURSE geo-files src/*.cc)
add_library(geo STATIC ${geo-files})
target_compile_features(geo PUBLIC cxx_std_17)
target_include_directories(geo PUBLIC include)
if (NOT MSVC)
  set_target_properties(geo PROPERTIES COMPILE_FLAGS "-Wall -Wextra")
else()
  target_compile_definitions(geo PUBLIC _USE_MATH_DEFINES)
endif()
target_link_libraries(geo ${Boost_LIBRARIES})

file(GLOB_RECURSE geo-test-files test/*.cc)

add_executable(geo-test ${geo-test-files})
target_compile_features(geo-test PUBLIC cxx_std_17)
if (NOT MSVC)
  set_target_properties(geo-test PROPERTIES COMPILE_FLAGS "-Wall -Wextra")
endif()
target_link_libraries(geo-test geo ${Boost_LIBRARIES} doctest)
