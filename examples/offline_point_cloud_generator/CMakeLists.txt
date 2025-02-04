cmake_minimum_required(VERSION 3.10)

project(offline_point_cloud_generator LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

find_package(OpenCV 4 REQUIRED COMPONENTS calib3d imgproc)

add_executable(offline_point_cloud_generator
        src/offline_point_cloud_generator.cpp
        )

target_include_directories(offline_point_cloud_generator
        PRIVATE
        include
        )

target_link_libraries(offline_point_cloud_generator
        PRIVATE
        common
        opencv_calib3d
        opencv_imgproc
        stdc++fs
        hammerhead::zmq_msgs
        hammerhead::zmq_opencv
        )

set_target_properties(offline_point_cloud_generator PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )