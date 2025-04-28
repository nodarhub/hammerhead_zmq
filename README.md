# Introduction

This repo contains several C++/CMake and Python examples demonstrating how to interact with Hammerhead using ZeroMQ (
ZMQ).

## C++ Dependencies

You will need:

1. A C++ compiler
    - Ubuntu:
        - You probably already have a compiler. If not, then the following will suffice:
          `sudo apt install build-essential`
    - Windows:
        - Generally, the easiest is to install the latest Community (a.k.a. *free*) version of Visual
          Studio: https://visualstudio.microsoft.com/vs/community/ . Note that unlike Visual Studio, Visual Studio Code
          does not come bundled with compillers.
2. CMake 3.11+:
    - Ubuntu >=20:
        - `sudo apt install cmake`
    - Ubuntu < 20:
        - The version in apt is likely 3.10, which is not sufficient. Follow the official directions for installing the
          most recent version of CMake: https://apt.kitware.com/
    - Windows:
        - If you will be using Visual Studio to manage your projects, then it might have CMake bundled with it.
          Otherwise, install one of the versions here: https://github.com/Kitware/CMake/releases/. For
          example: https://github.com/Kitware/CMake/releases/download/v3.26.2/cmake-3.26.2-windows-x86_64.msi
3. To interact with Hammerhead, you don't need OpenCV. However, a lot of our examples use it. Therefore, you should
   consider installing it:
    - Ubuntu:
        - `sudo apt install libopencv-dev`
    - Windows:
        - Download version 4.6.0 or newer from https://opencv.org/releases/

## Project Structure

The `zmq_msgs` folder contains the code for the `zmq_msgs` target, which defines how objects are sent and received via
ZeroMQ on the network. It also defines other important networking information, such as which ports are used for which
topics.

The `examples` folder contains some examples that you should look at. These demonstrate how one should interact with
Hammerhead. We envision that you will use these examples as a jumping-off point for your application.

We suggest that you start by examining the code and README's in the individual example directories for more details
about what each example does.

## C++ Usage

To use the message types in a new project, you can copy `zmq_msgs/cpp` folder into your project, and then modify your
`CMakeLists.txt` to link to this target:

    add_subdirectory(zmq_msgs/cpp)
    target_link_libraries(
            my_target
            PRIVATE
            hammerhead::zmq_msgs
    )

The other examples that we provide, such as the `image_viewer` target, demonstrate how to do this.

## Python Usage

We set up the python examples so that you can `cd` into the example folder and directly run the code:

    $ cd examples/python/image_viewer/image_viewer
    $ python image_viewer.py 

Alternatively, we recommend that you create a virtual environment somewhere on your system,
where you can install the requirements, the `zmq_msgs` package, and the examples of your choosing:

    cd ~/testing
    python3 -m venv nodarenv
    source nodarenv/bin/activate
    python -m pip install -r /path/to/hammerhead_zmq/requirements.txt
    python -m pip install /path/to/hammerhead_zmq/zmq_msgs/python
    python -m pip install /path/to/hammerhead_zmq/examples/python/image_viewer

    # Now you can run the example: 
    image_viewer

## Note on Disparity to 3D Point Cloud conversion

To convert disparity images to 3D point clouds, we follow the standard stereo reconstruction pipeline:

### Disparity Scaling

The disparity is in Q12.4 format. We scale the disparity by `1 / 16.0` to get the disparity in float32 format:

    disparity_scaled = 1 * disparity / 16.0

### 3D Reprojection

The scaled disparity map is reprojected into 3D space using OpenCV’s `cv2.reprojectImageTo3D()` and a 4×4 reprojection
matrix `Q`:

    points_3d = cv2.reprojectImageTo3D(disparity_scaled, Q)

A negative translation vector (`Tx < 0`) is used when creating the `Q` matrix to conform the definition in OpenCV. This
ensures that the point cloud is generated in a consistent right-handed coordinate frame. As a result, the entire last
row of `Q` must be negated before passing to the `cv2.reprojectImageTo3D()` call:

    Q[3, :] = -Q[3, :]

This conversion scheme has been used in the following examples:

- examples/cpp/offline_point_cloud_generator
- examples/cpp/point_cloud_soup_recorder
- examples/python/point_cloud_soup_recorder