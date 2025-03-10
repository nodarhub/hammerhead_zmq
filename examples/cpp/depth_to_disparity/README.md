This example shows how to convert `.exr` depth images to `.tiff` disparity images. The disparity data is needed to generate point clouds in the Nodar Viewer.

WARNING: The depth maps recorded by Hammerhead are in the EXR format. This example converts those images into lossless `.tiff` images. To do that, this example assumes that you have a version of OpenCV installed with EXR support. This seems to be
the case for the OpenCV version in apt on many Ubuntu X86-64 installations. However, the OpenCV version in apt on the
Orin does not seem to have EXR support.

To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

To run this example, you need to provide the path to a directory of data saved by Hammerhead. It is necessary to have both the `depth` and `details` folders in the data directory:

    # Linux
    ./depth_to_disparity data_directory [output_directory]

Note that by default, the output_directory will be a `disparity` folder in your data_directory. If you already have this directory, the files inside will be overriden.