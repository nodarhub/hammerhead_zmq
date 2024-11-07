Nodar generates high resolution point clouds; much higher than what you would find on a traditional lidar. Because of this, we need to be very careful about how data is passed on the network. In particular, instead of passing a traditional point cloud, we supply a much smaller representation called a `PointCloudSoup` from which a point cloud can be reconstructed on the client machine. The class, as well as functions to read and write the class are provided in the header `point_cloud_soup.hpp`.   

This example shows how to use that header. Specifically, it shows how to subscribe to `PointCloudSoup` messages being sent by Hammerhead, and construct point clouds on your machine. In this example, those point clouds are then saved as `.ply` files in the `point_clouds` folder. You can then open the `.ply` files in other programs, such as CloudCompare. 

To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the IP address of the ZMQ source (almost certainly the Orin's IP address):

    # Linux
    ./point_cloud_generator src_ip

    # Windows
    ./Release/point_cloud_generator.exe src_ip

Note that if you specify an incorrect IP address or run this example when Hammerhead is not running, then ZMQ will attempt to subscribe, and nothing will happen. It will appear like the binary is just waiting.

To kill this example, just press CTRL+C.