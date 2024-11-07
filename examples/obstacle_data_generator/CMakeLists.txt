cmake_minimum_required(VERSION 3.10)

project(obstacle_data_generator LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

find_package(OpenCV 4 REQUIRED COMPONENTS core)

add_executable(obstacle_data_generator
        src/obstacle_data_generator.cpp
)

target_link_libraries(obstacle_data_generator
        PRIVATE
        hammerhead::zmq_msgs
)

set_target_properties(obstacle_data_generator PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)