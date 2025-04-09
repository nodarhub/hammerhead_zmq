This example shows how to subscribe to `PointCloud` messages being sent by Hammerhead, 
and then save them as `.ply` files in the `point_clouds` folder. 
You can then open the `.ply` files in other programs, such as CloudCompare. 
Note that due to the amount of bandwidth required, 
you should think twice before enabling point cloud streaming in hammerhead.

To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the IP address of the ZMQ source (the device running hammerhead):

    # Linux
    ./point_cloud_recorder src_ip

    # Windows
    ./Release/point_cloud_recorder.exe src_ip

Note that if you specify an incorrect IP address or run this example when Hammerhead is not running, then ZMQ will
attempt to subscribe, and nothing will happen. It will appear like the binary is just waiting.

To kill this example, just press CTRL+C.

There is an additional example `point_cloud_rgb_recorder` demonstrating how to receive RGB point clouds. 
However, the bandwidth requirements are even higher. Use with caution.
