cmake_minimum_required(VERSION 3.10)

project(network LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
message(STATUS "Build type " ${CMAKE_BUILD_TYPE})
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -static-libasan")

    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=0)
else ()
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_INSTALL_RPATH "./lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif ()

# link with static lib of spdlog
add_compile_definitions(
        SPDLOG_COMPILED_LIB
)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

include_directories(include)
include_directories(src)

link_directories(lib)

add_subdirectory(src)
add_subdirectory(test)
