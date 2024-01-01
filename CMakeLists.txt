cmake_minimum_required(VERSION 3.10)

project(network LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
message(STATUS "Build type " ${CMAKE_BUILD_TYPE})
set(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS} -g -O0")
    #set(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS} -fsanitize=address -static-libasan")
endif ()

include_directories(include)
include_directories(src)
link_directories(lib)

add_subdirectory(src)
add_subdirectory(test)
