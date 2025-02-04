cmake_minimum_required(VERSION 3.10)

project(depth_to_disparity LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

add_executable(depth_to_disparity
        src/depth_to_disparity.cpp
        )

target_link_libraries(depth_to_disparity
        PRIVATE
        common
        )

set_target_properties(depth_to_disparity PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )