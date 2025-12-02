cmake_minimum_required(VERSION 3.10)

project(qa_findings_viewer LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

add_executable(qa_findings_viewer
        src/qa_findings_viewer.cpp
)

target_link_libraries(qa_findings_viewer
        PRIVATE
        hammerhead::zmq_msgs
        hammerhead::zmq_opencv
)

set_target_properties(qa_findings_viewer PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)