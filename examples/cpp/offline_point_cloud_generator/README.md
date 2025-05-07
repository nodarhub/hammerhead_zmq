This example shows how to turn the data saved by Hammerhead into point clouds, which are then saved as `.ply` files. You
can open the `.ply` files in other programs, such as CloudCompare.

WARNING: The depth maps recorded by Hammerhead are in the TIFF format, but previously were recorded as EXR. 
This example converts those images into point clouds. 
To do that, this example assumes that you have a version of OpenCV installed with EXR support. 
This seems to be the case for the OpenCV version in `apt` on many Ubuntu X86-64 installations, but not on ARM systems.


To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the path to a directory of data saved by Hammerhead:

    # Linux
    ./offline_point_cloud_generator data_directory [output_directory]

    # Windows
    ./Release/offline_point_cloud_generator.exe data_directory [output_directory]

Note that by default, the output_directory will be a `point_clouds` folder in your data_directory. 