cmake_minimum_required(VERSION 3.10)

project(point_cloud_soup_recorder LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

find_package(OpenCV 4 REQUIRED COMPONENTS calib3d imgproc)

add_executable(point_cloud_soup_recorder
        src/point_cloud_soup_recorder.cpp
)

target_link_libraries(point_cloud_soup_recorder
        PRIVATE
        opencv_calib3d
        opencv_imgproc
        #stdc++fs
        hammerhead::zmq_msgs
        hammerhead::zmq_opencv
)

set_target_properties(point_cloud_soup_recorder PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)