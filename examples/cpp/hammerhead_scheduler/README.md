# Introduction
This example demonstrates how to schedule hammerhead using ZMQ.
Specifically, under normal conditions, hammerhead runs without external input. 
However, if the machine running hammerhead is running other intensive processes, 
then you may want to schedule when hammerhead runs in order to avoid resource allocation issues.
This example demonstrates how to do that. 

## How it works
In the hammerhead `master_config.ini` file, there is the parameter `wait_for_scheduler`.  
When enabled, hammerhead will process a frame and then send a scheduler request with the frame number
indicating that hammerhead will stop and wait until a reply from a scheduler is received. 
To avoid deadlocking, the `master_config.ini` also exposes `wait_for_scheduler_timeout_ms`, 
which tells hammerhead how many milliseconds to wait on a scheduler before giving up and moving to the next frame. 

## Quick Start
This example implements a basic scheduler that can interact with hammerhead. 
To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

This will build the binary `hammerhead_scheduler`. To run this example, you should provide the IP address of the device running hammerhead:

    # Linux
    ./hammerhead_scheduler src_ip

    # Windows
    ./Release/hammerhead_scheduler src_ip

Note that if you specify an incorrect IP address or run this example when Hammerhead is not running, then nothing will happen. It will appear like the binary is just waiting.

To kill this example, just press CTRL+C.