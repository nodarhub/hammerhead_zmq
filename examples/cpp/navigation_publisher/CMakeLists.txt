cmake_minimum_required(VERSION 3.10)

project(navigation_publisher LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

add_executable(navigation_publisher
        src/navigation_publisher.cpp
)

target_include_directories(navigation_publisher
        PRIVATE
        include
)

target_link_libraries(navigation_publisher
        PRIVATE
        hammerhead::zmq_msgs
)

set_target_properties(navigation_publisher PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)