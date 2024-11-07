cmake_minimum_required(VERSION 3.10)

project(point_cloud_stream LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

# Example with no RGB info
add_executable(point_cloud_stream
        src/point_cloud_stream.cpp
)
target_link_libraries(point_cloud_stream
        PRIVATE
        #stdc++fs
        hammerhead::zmq_msgs
)
set_target_properties(point_cloud_stream PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# RGB example
add_executable(point_cloud_rgb_stream
        src/point_cloud_rgb_stream.cpp
)
target_link_libraries(point_cloud_rgb_stream
        PRIVATE
        #stdc++fs
        hammerhead::zmq_msgs
)
set_target_properties(point_cloud_rgb_stream PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)