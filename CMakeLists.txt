cmake_minimum_required(VERSION 3.11)

project(hammerhead_zmq VERSION 0.1.0 LANGUAGES CXX)

add_subdirectory(submodules/yaml-cpp)
add_subdirectory(zmq_msgs/cpp)
add_subdirectory(zmq_opencv/cpp)

option(BUILD_EXAMPLES "Whether or not to build examples" ON)
if (BUILD_EXAMPLES)
    add_subdirectory(examples/cpp)
endif ()