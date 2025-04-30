This example shows how to subscribe to `ObstacleData` messages being sent by Hammerhead,
and then save them as `.txt` files in the `obstacle_datas` folder.
The header of each generated file contains information about the order of parameters.

The obstacle data is represented in the XZ plane,
where each obstacle is defined by a bounding box and a velocity vector.
Because this data is only in the XZ plane, there is no concept of *top* and *bottom* planes.
Similarly, the velocity vector does not have any vertical component (along what we would call the Y-axis).

To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the IP address of the ZMQ source (the device running Hammerhead):

    # Linux
    ./obstacle_data_recorder src_ip

    # Windows
    ./Release/obstacle_data_recorder.exe src_ip

Note that if you specify an incorrect IP address or run this example when Hammerhead is not running,
then ZMQ will attempt to subscribe, and nothing will happen.
It will appear like the binary is just waiting.

To kill this example, just press CTRL+C.