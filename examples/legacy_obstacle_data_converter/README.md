This example shows how to convert `bounding_boxes_and_velocities` legacy data  to `tracked-objects` data. The `tracked-objects` data is needed to display object's bounding boxes in the Nodar Viewer.

To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the path to a directory of data saved by Hammerhead. It is necessary to have the `bounding_boxes_and_velocities` folder in the data directory:


    # Linux
    ./legacy_obstacle_data_converter data_directory [output_directory]

Note that by default, the output_directory will be a `tracked-objects` folder in your data_directory. If you already have this directory, the files inside will be overriden.