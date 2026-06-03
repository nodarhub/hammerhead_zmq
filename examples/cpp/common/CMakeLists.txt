cmake_minimum_required(VERSION 3.10)

project(common LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

# `common_base` holds header-only helpers with no external dependencies.
# Link this target from examples that only need lightweight utilities
# (frameString, dateString, tqdm, getFiles) without pulling in OpenCV/yaml-cpp.
add_library(common_base INTERFACE)

target_sources(common_base INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/date_string.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fps.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/frame_string.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/get_files.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tqdm.hpp
)

target_include_directories(common_base INTERFACE
        include
)

set_target_properties(common_base PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# `common` pulls in everything in `common_base` plus the headers that depend
# on OpenCV (safe_load) and yaml-cpp (details_parameters).
if (NOT TARGET opencv_imgcodecs)
    find_package(OpenCV 4 REQUIRED COMPONENTS imgcodecs)
endif ()

add_library(common INTERFACE)

target_sources(common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/details_parameters.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/safe_load.hpp
)

target_link_libraries(common
        INTERFACE
        common_base
        opencv_imgcodecs
        yaml-cpp::yaml-cpp
)

set_target_properties(common PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
