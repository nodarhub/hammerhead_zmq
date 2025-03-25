cmake_minimum_required(VERSION 3.10)

project(toggle_recording LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

add_executable(toggle_recording
        src/toggle_recording.cpp
)

target_link_libraries(toggle_recording
        PRIVATE
        hammerhead::zmq_msgs
)

set_target_properties(toggle_recording PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)