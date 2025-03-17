This example shows how to publish the left and right stereo images stacked vertically (also called topbot images) from
the disk to Hammerhead.

To build this example, follow the traditional CMake process from the root of the repository:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the path to the topbot data directory, the port number to publish the images to
and optionally the pixel format:

    # Linux
    ./topbot_publisher <topbot_data_directory> <port_number> [pixel_format]

    # Windows
    ./Release/topbot_publisher.exe <topbot_data_directory> <port_number> [pixel_format]

Note that the images inside the `topbot_data_directory` should be sequentially numbered. The images should be in the
`.tiff` format.

Supported pixel formats:

- BGR
- Bayer_RGGB
- Bayer_GRBG
- Bayer_BGGR
- Bayer_GBRG

Note that if the `pixel_format` is not provided, the default pixel format is `BGR`.