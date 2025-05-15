cmake_minimum_required(VERSION 3.10)

project(common LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

find_package(OpenCV 4 REQUIRED COMPONENTS imgcodecs)

add_library(common INTERFACE)

target_sources(common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/details_parameters.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/get_files.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/safe_load.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tqdm.hpp
        )

target_include_directories(common INTERFACE
        include
        )

target_link_libraries(common
        INTERFACE
        opencv_imgcodecs
        yaml-cpp::yaml-cpp
        )

set_target_properties(common PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )