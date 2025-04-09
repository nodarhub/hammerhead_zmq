cmake_minimum_required(VERSION 3.10)

project(point_cloud_recorder LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

# Example with no RGB info
add_executable(point_cloud_recorder
        src/point_cloud_recorder.cpp
)
target_link_libraries(point_cloud_recorder
        PRIVATE
        #stdc++fs
        hammerhead::zmq_msgs
)
set_target_properties(point_cloud_recorder PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# RGB example
add_executable(point_cloud_rgb_recorder
        src/point_cloud_rgb_recorder.cpp
)
target_link_libraries(point_cloud_rgb_recorder
        PRIVATE
        #stdc++fs
        hammerhead::zmq_msgs
)
set_target_properties(point_cloud_rgb_recorder PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)