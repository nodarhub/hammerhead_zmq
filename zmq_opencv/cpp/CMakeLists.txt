cmake_minimum_required(VERSION 3.11)

if (NOT (TARGET opencv_core AND TARGET opencv_imgproc))
    find_package(OpenCV 4 QUIET COMPONENTS core imgproc)
endif ()

if (TARGET opencv_core AND TARGET opencv_imgproc)
    add_library(zmq_opencv INTERFACE)
    add_library(hammerhead::zmq_opencv ALIAS zmq_opencv)

    target_include_directories(zmq_opencv INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    target_link_libraries(zmq_opencv INTERFACE opencv_core opencv_imgproc)
else ()
    message(STATUS "OpenCV not found, hammerhead::zmq_opencv will not be built.")
endif ()