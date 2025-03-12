This example shows how to publish the left and right stereo images stacked vertically (also called topbot images) from the disk to Hammerhead.

To build this example, follow the traditional CMake process from the root of the repository:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the path to the topbot data directory and the port number to publish the images to:

    # Linux
    ./topbot_publisher <topbot_data_directory> <port_number>

    # Windows
    ./Release/topbot_publisher.exe <topbot_data_directory> <port_number>

Note that the images inside the topbot_data_directory should be sequentially numbered. The images should be in the .tiff format.
