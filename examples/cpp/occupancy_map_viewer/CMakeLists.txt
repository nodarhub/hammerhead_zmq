cmake_minimum_required(VERSION 3.10)

project(occupancy_map_viewer LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

find_package(OpenCV 4 REQUIRED COMPONENTS core highgui)

add_executable(occupancy_map_viewer
        src/occupancy_map_viewer.cpp
)

target_link_libraries(occupancy_map_viewer
        PRIVATE
        hammerhead::zmq_msgs
        hammerhead::zmq_opencv
        opencv_highgui
)

set_target_properties(occupancy_map_viewer PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# Stats version without OpenCV - just prints metadata and occupied cell count
add_executable(occupancy_map_stats
        src/occupancy_map_stats.cpp
)

target_link_libraries(occupancy_map_stats
        PRIVATE
        hammerhead::zmq_msgs
)

set_target_properties(occupancy_map_stats PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)