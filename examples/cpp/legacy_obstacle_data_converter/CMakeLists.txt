cmake_minimum_required(VERSION 3.10)

project(legacy_obstacle_data_converter LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

add_executable(legacy_obstacle_data_converter
        src/legacy_obstacle_data_converter.cpp
        )

target_link_libraries(legacy_obstacle_data_converter
        PRIVATE
        common
        )

set_target_properties(legacy_obstacle_data_converter PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )