cmake_minimum_required(VERSION 3.10)

project(topbot_publisher LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

find_package(OpenCV 4 REQUIRED COMPONENTS highgui imgproc)

add_executable(topbot_publisher
        src/topbot_publisher.cpp
)

target_link_libraries(topbot_publisher
    PRIVATE
        opencv_highgui
        opencv_imgproc
        hammerhead::zmq_msgs
        hammerhead::zmq_opencv
)

set_target_properties(topbot_publisher PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)