cmake_minimum_required(VERSION 3.11)

project(zmq_msgs VERSION 0.1.0 LANGUAGES CXX)

set(disabled_opts
        BUILD_TESTS
        CPPZMQ_BUILD_TESTS
        ENABLE_CPACK
        ENABLE_DRAFTS
        ENABLE_RADIX_TREE
        ENABLE_WS
        WITH_DOCS
        WITH_LIBSODIUM
        WITH_PERF_TOOL
        WITH_TLS
        ZMQ_BUILD_TESTS
)

foreach (disabled_opt in ${disabled_opts})
    SET(${disabled_opt} OFF CACHE BOOL "${disabled_opt}")
endforeach ()

option(EXCLUDE_LIBZMQ_FROM_ALL "Exclude libzmq from ALL build target" OFF)
option(EXCLUDE_CPPZMQ_FROM_ALL "Exclude cppzmq from ALL build target" OFF)

find_package(ZeroMQ 4.3.5 QUIET)
if (ZeroMQ_FOUND)
    message(STATUS "libzmq was found on your system")
else ()
    message(STATUS "libzmq was not found on your system. Building the submodule")
    if (EXCLUDE_LIBZMQ_FROM_ALL)
        add_subdirectory(submodules/libzmq EXCLUDE_FROM_ALL)
    else ()
        add_subdirectory(submodules/libzmq)
    endif ()
endif ()

if (EXCLUDE_CPPZMQ_FROM_ALL)
    add_subdirectory(submodules/cppzmq EXCLUDE_FROM_ALL)
else ()
    add_subdirectory(submodules/cppzmq)
endif ()
if (NOT TARGET cppzmq)
    message(FATAL_ERROR "cppzmq could not be added as a target")
endif ()

add_library(zmq_msgs INTERFACE)
add_library(hammerhead::zmq_msgs ALIAS zmq_msgs)

target_include_directories(zmq_msgs INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(zmq_msgs INTERFACE cppzmq)