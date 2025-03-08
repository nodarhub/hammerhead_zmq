cmake_minimum_required(VERSION 3.10)

project(hammerhead_scheduler LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

add_executable(
        hammerhead_scheduler
        src/hammerhead_scheduler.cpp
)

target_link_libraries(
        hammerhead_scheduler
        PRIVATE
        hammerhead::zmq_msgs
)

set_target_properties(
        hammerhead_scheduler
        PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)