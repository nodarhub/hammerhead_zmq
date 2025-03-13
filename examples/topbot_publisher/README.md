This example shows how to publish the left and right stereo images stacked vertically (also called topbot images) from the disk to Hammerhead.

To build this example, follow the traditional CMake process from the root of the repository:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the path to the topbot data directory and the topbot topic name or port to publish the images to:

    # Linux
    ./topbot_publisher <topbot_data_directory> [topbot_topic_or_port]

    # Windows
    ./Release/topbot_publisher.exe <topbot_data_directory> [topbot_topic_or_port]

Note that the images inside the topbot_data_directory should be sequentially numbered. The images should be in the .tiff format.

The file `topic_ports.hpp` defines the `topic->port` mappings for topbots. For example, in that file, you will find
the entry

    "external/topbot/bayer_rggb", 55001

which indicates that the Bayer RGGB format images will be sent on port 55001. 

You can specify either the port or topic name:

    ./topbot_publisher <topbot_data_directory> 55001
    ./topbot_publisher <topbot_data_directory> external/topbot/bayer_rggb

Note that the parameter `topbot_topic_or_port` should be one of the `EXTERNAL_TOPBOT_TOPICS` in `topic_ports.hpp`.

To kill this example, just press CTRL+C.

Some currently valid topbot topic names are:

    external/topbot/bgr
    external/topbot/bayer_rggb
    external/topbot/bayer_grbg
    external/topbot/bayer_bggr
    external/topbot/bayer_gbrg
