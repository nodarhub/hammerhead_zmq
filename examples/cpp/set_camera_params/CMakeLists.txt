cmake_minimum_required(VERSION 3.10)

project(set_camera_params LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE was not set by the user. Defaulting to ${CMAKE_BUILD_TYPE}")
endif ()

foreach (TOPIC IN ITEMS exposure gain)
    add_executable(set_${TOPIC}
        src/set_camera_params.cpp
    )

    target_compile_definitions(set_${TOPIC} PUBLIC TOPIC_${TOPIC})

    target_link_libraries(set_${TOPIC}
        PRIVATE
        hammerhead::zmq_msgs
    )

    set_target_properties(set_${TOPIC} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
endforeach ()