cmake_minimum_required(VERSION 3.10)

project(image_recorder LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

find_package(OpenCV 4 REQUIRED COMPONENTS highgui imgproc)

add_executable(image_recorder
        src/image_recorder.cpp
)

target_link_libraries(image_recorder
        PRIVATE
        opencv_highgui
        opencv_imgproc
        hammerhead::zmq_msgs
        hammerhead::zmq_opencv
)

set_target_properties(image_recorder PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)