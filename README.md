# Hammerhead ZMQ Client Library

> A comprehensive Python and C++ client library for interfacing with the Hammerhead stereo vision system via ZeroMQ

[![License](images/shark_license.png)](LICENSE)

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Message Types & Ports](#message-types--ports)
- [Installation](#installation)
- [Project Structure](#project-structure)
- [Examples & Tutorials](#examples--tutorials)
- [C++ Integration](#c-integration)
- [Python Integration](#python-integration)
- [3D Coordinate System & Point Cloud Conversion](#3d-coordinate-system--point-cloud-conversion)
- [API Reference](#api-reference)
- [Best Practices & Tips](#best-practices--tips)

## Overview

The Hammerhead system is a high-performance stereo vision processing unit that publishes various types of data over ZeroMQ. This library provides easy-to-use APIs for receiving and processing:

- **Stereo Images** - Raw and rectified left/right camera feeds
- **Depth Data** - Disparity maps and color-blended depth images  
- **Point Clouds** - 3D point cloud data with or without RGB information
- **Obstacle Detection** - Real-time obstacle data with bounding boxes
- **Camera Control** - Parameter adjustment and scheduling

### Key Features

| Feature | Description |
|---------|-------------|
| 🐍 **Python Ready** | Complete Python package with examples and utilities |
| ⚡ **High Performance C++** | Optimized C++ implementation for real-time applications |
| 🔌 **ZeroMQ Protocol** | Efficient, low-latency messaging with automatic reconnection |

## Quick Start

### Python Installation & Usage

```bash
# Install the package
pip install -e .

# View live stereo images from Hammerhead device at 192.168.1.100
python examples/python/image_viewer/image_viewer/image_viewer.py 192.168.1.100 nodar/left/image_raw

# Record point clouds to PLY files
python examples/python/point_cloud_recorder/point_cloud_recorder/point_cloud_recorder.py 192.168.1.100

# Record obstacle detection data
python examples/python/obstacle_data_recorder/obstacle_data_recorder/obstacle_data_recorder.py 192.168.1.100
```

### C++ Build & Usage

```bash
# Build all examples
mkdir build && cd build
cmake .. && cmake --build . --config Release

# View live images
./examples/cpp/image_viewer/image_viewer 192.168.1.100 nodar/left/image_raw

# Record point clouds
./examples/cpp/point_cloud_recorder/point_cloud_recorder 192.168.1.100
```

## Message Types & Ports

Hammerhead publishes data using structured message types over predefined ZMQ ports:

### Image Streams
| Port | Topic | Description | Message Type |
|------|-------|-------------|--------------|
| 9800 | `nodar/left/image_raw` | Raw left camera feed | `StampedImage` |
| 9801 | `nodar/right/image_raw` | Raw right camera feed | `StampedImage` |
| 9802 | `nodar/left/image_rect` | Rectified left image | `StampedImage` |
| 9803 | `nodar/right/image_rect` | Rectified right image | `StampedImage` |
| 9804 | `nodar/disparity` | Disparity map (Q12.4 format) | `StampedImage` |
| 9805 | `nodar/color_blended_depth/image_raw` | Color-coded depth visualization | `StampedImage` |
| 9813 | `nodar/topbot_raw` | Top/bottom camera feed | `StampedImage` |

### 3D Data Streams
| Port | Topic | Description                        | Message Type |
|------|-------|------------------------------------|--------------|
| 9806 | `nodar/point_cloud_soup` | Compact point cloud representation | `PointCloudSoup` |
| 9809 | `nodar/point_cloud` | Ordered point cloud | `PointCloud` |
| 9810 | `nodar/point_cloud_rgb` | RGB point cloud                    | `PointCloudRGB` |

### Detection & Control
| Port | Topic | Description | Message Type |
|------|-------|-------------|--------------|
| 9807 | `nodar/set_exposure` | Camera exposure control | `Control` |
| 9808 | `nodar/set_gain` | Camera gain control | `Control` |
| 9811 | `nodar/recording` | Recording on/off control | `SetBool` |
| 9812 | `nodar/obstacle` | Obstacle detection data | `ObstacleData` |
| 9814 | `nodar/wait` | Scheduler control | `SetBool` |

## Installation

### Python Dependencies

```bash
# Install core dependencies
pip install -r requirements.txt

# Install ZMQ message types
pip install -e zmq_msgs/python

# Install specific examples (optional)
pip install -e examples/python/image_viewer
pip install -e examples/python/point_cloud_recorder
```

**Python Requirements:**
- Python 3.10+
- NumPy ≥1.26.0
- OpenCV ≥4.8.0
- PyZMQ
- PyYAML
- tqdm

### C++ Dependencies

**Ubuntu:**
```bash
# Install build tools
sudo apt install build-essential cmake

# Install OpenCV (optional but recommended)
sudo apt install libopencv-dev
```

**Windows:**
- **Visual Studio Community** - [Download](https://visualstudio.microsoft.com/vs/community/)
- **CMake 3.11+** - [Download](https://github.com/Kitware/CMake/releases/)
- **OpenCV 4.6.0+** - [Download](https://opencv.org/releases/)

## Project Structure

The `zmq_msgs` folder contains the code for the `zmq_msgs` target, which defines how objects are sent and received via
ZeroMQ on the network. It also defines other important networking information, such as which ports are used for which
topics.

The `examples` folder contains comprehensive examples that demonstrate how to interact with Hammerhead. We envision that you will use these examples as a jumping-off point for your application.

We suggest that you start by examining the code and README's in the individual example directories for more details
about what each example does.

## Examples & Tutorials

### 🐍 Python Examples

Python examples provide easy-to-use scripts for common Hammerhead integration tasks. 

#### Visualization Examples
- **[Image Viewer](examples/python/image_viewer/README.md)** - Real-time OpenCV viewer for stereo images, disparity maps, and depth data

#### Data Capture Examples
- **[Image Recorder](examples/python/image_recorder/README.md)** - Record images from any Hammerhead stream to disk as TIFF files
- **[Point Cloud Recorder](examples/python/point_cloud_recorder/README.md)** - Subscribe to point cloud messages and save them as PLY files
- **[Point Cloud Soup Recorder](examples/python/point_cloud_soup_recorder/README.md)** - Stream the reduced-bandwidth PointCloudSoup messages, convert to point clouds, and save as PLY files
- **[Obstacle Data Recorder](examples/python/obstacle_data_recorder/README.md)** - Record real-time obstacle detection data

#### Processing Examples
- **[Depth to Disparity Converter](examples/python/depth_to_disparity/README.md)** - Convert depth images to disparity format
- **[Disparity to Point Cloud](examples/python/disparity_to_ordered_point_cloud/README.md)** - Convert stored disparity images to ordered 3D point clouds

#### Control Examples
- **[Hammerhead Scheduler](examples/python/hammerhead_scheduler/README.md)** - Control Hammerhead's processing schedule
- **[Topbot Publisher](examples/python/topbot_publisher/README.md)** - Publish images to Hammerhead's top/bottom camera topic

### ⚡ C++ Examples

High-performance C++ implementations for real-time applications and system integration.

#### Visualization Examples
- **[Image Viewer](examples/cpp/image_viewer/README.md)** - Real-time OpenCV viewer for stereo images, disparity maps, and depth data

#### Data Capture Examples
- **[Image Recorder](examples/cpp/image_recorder/README.md)** - Record images from any Hammerhead stream to disk as TIFF files
- **[Point Cloud Recorder](examples/cpp/point_cloud_recorder/README.md)** - Subscribe to point cloud messages and save them as PLY files
- **[Point Cloud Soup Recorder](examples/cpp/point_cloud_soup_recorder/README.md)** - Stream the reduced-bandwidth PointCloudSoup messages, convert to point clouds, and save as PLY files
- **[Obstacle Data Recorder](examples/cpp/obstacle_data_recorder/README.md)** - Record real-time obstacle detection data

#### Processing Examples
- **[Offline Point Cloud Generator](examples/cpp/offline_point_cloud_generator/README.md)** - Batch processing of disparity images
- **[Depth to Disparity Converter](examples/cpp/depth_to_disparity/README.md)** - Convert depth images to disparity format
- **[Legacy Obstacle Data Converter](examples/cpp/legacy_obstacle_data_converter/README.md)** - Convert legacy obstacle data formats

#### Control Examples
- **[Hammerhead Scheduler](examples/cpp/hammerhead_scheduler/README.md)** - Control Hammerhead's processing schedule
- **[Camera Parameter Control](examples/cpp/set_camera_params/README.md)** - Real-time camera parameter adjustment
- **[Recording Toggle](examples/cpp/toggle_recording/README.md)** - Simple utility to start/stop Hammerhead recording
- **[Topbot Publisher](examples/cpp/topbot_publisher/README.md)** - Publish images to Hammerhead's top/bottom camera topic

### Common Integration Workflows

#### 🎥 Image Processing Pipeline
1. Start with **Image Viewer** to verify camera feeds
2. Use **Image Recorder** to capture datasets
3. Process images with custom algorithms

#### 🌐 3D Reconstruction Workflow
1. Subscribe to `PointCloudSoup` messages to reduce network bandwidth 
2. Reconstruct point clouds
3. Process images with custom algorithms
4. Integrate with 3D processing frameworks

#### 🚗 Obstacle Detection Integration
1. Use **Obstacle Data Recorder** to understand data format
2. Implement real-time processing of obstacle messages
3. Integrate with path planning or control systems
4. Add custom filtering or tracking algorithms

## C++ Integration

To use the message types in a new project, you can copy `zmq_msgs/cpp` folder into your project, and then modify your
`CMakeLists.txt` to link to this target:

```cmake
add_subdirectory(zmq_msgs/cpp)
target_link_libraries(
        my_target
        PRIVATE
        hammerhead::zmq_msgs
)
```

Refer to the extensive examples for some practical examples.

## Python Integration

We set up the python examples so that you can `cd` into the example folder and directly run the code:

```bash
$ cd examples/python/image_viewer/image_viewer
$ python image_viewer.py 
```

Alternatively, we recommend that you create a virtual environment somewhere on your system,
where you can install the requirements, the `zmq_msgs` package, and the examples of your choosing:

```bash
cd ~/testing
python3 -m venv nodarenv
source nodarenv/bin/activate
python -m pip install -r /path/to/hammerhead_zmq/requirements.txt
python -m pip install /path/to/hammerhead_zmq/zmq_msgs/python
python -m pip install /path/to/hammerhead_zmq/examples/python/image_viewer

# Now you can run the example: 
image_viewer
```

## 3D Coordinate System & Point Cloud Conversion

Hammerhead follows standard stereo reconstruction principles for converting disparity to 3D point clouds:

### Disparity Scaling

The disparity is in Q12.4 format. We scale the disparity by `1 / 16.0` to get the disparity in `float32` format:

```python
disparity_scaled = disparity.astype(np.float32) / 16.0
```

### 3D Reprojection

The scaled disparity map is reprojected into 3D space using OpenCV's `cv2.reprojectImageTo3D()` and a 4×4 reprojection
matrix `Q`:

```python
# Important: Negate the last row for correct coordinate frame
Q_corrected = Q.copy()
Q_corrected[3, :] = -Q_corrected[3, :]

# Reproject to 3D
points_3d = cv2.reprojectImageTo3D(disparity_scaled, Q_corrected)
```

A negative translation vector (`Tx < 0`) is used when creating the `Q` matrix to conform to the definition in OpenCV. This
ensures that the point cloud is generated in a consistent right-handed coordinate frame. As a result, the entire last
row of `Q` must be negated before passing to the `cv2.reprojectImageTo3D()` call.

This conversion scheme has been used in the following examples:

- **[Offline Point Cloud Generator](examples/cpp/offline_point_cloud_generator/README.md)** - C++ batch processing
- **[Point Cloud Soup Recorder](examples/cpp/point_cloud_soup_recorder/README.md)** - C++ real-time recording
- **[Point Cloud Soup Recorder](examples/python/point_cloud_soup_recorder/README.md)** - Python real-time recording
- **[Disparity to Point Cloud](examples/python/disparity_to_ordered_point_cloud/README.md)** - Python offline processing

## API Reference

### Message Types

All Hammerhead messages use a versioned protocol with the `MessageInfo` header structure.

#### StampedImage
Used for all image data including raw stereo images, rectified images, and disparity maps.

```python
from zmq_msgs.image import StampedImage
import zmq

# Create ZMQ subscriber
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect(f"tcp://{ip_address}:{port}")
socket.setsockopt(zmq.SUBSCRIBE, b"")

# Receive and decode image
buffer = socket.recv()
stamped_image = StampedImage()
stamped_image.read(buffer)

# Access image data
cv2.imshow("Image", stamped_image.img)
print(f"Frame ID: {stamped_image.frame_id}")
print(f"Timestamp: {stamped_image.time}")
```

#### ObstacleData
Contains real-time obstacle detection information with bounding boxes and velocity vectors.

```python
from zmq_msgs.obstacle_data import ObstacleData
import zmq

# Create ZMQ subscriber
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect(f"tcp://{ip_address}:9812")
socket.setsockopt(zmq.SUBSCRIBE, b"")

# Receive and decode obstacle data
buffer = socket.recv()
obstacle_data = ObstacleData()
obstacle_data.read(buffer)

# Process obstacles
for obstacle in obstacle_data.obstacles:
    print(f"Bounding box: {obstacle.bounding_box.points}")
    print(f"Velocity: ({obstacle.velocity.x:.2f}, {obstacle.velocity.z:.2f}) m/s")
```

**Coordinate System:** Obstacle data is represented in the XZ plane (bird's eye view):
- **X axis**: Left/right relative to camera
- **Z axis**: Forward/backward from camera
- **No Y component**: Height information not included

## Best Practices & Tips

### 🔧 Performance
- Use C++ for real-time applications
- Monitor network bandwidth with point clouds
- Implement connection timeouts
- Consider message buffering for high-frequency data

### 🛡️ Reliability
- Always validate message types and versions
- Implement automatic reconnection logic
- Handle partial message reception
- Add logging for debugging network issues

### 🌐 Networking
- Test with different network conditions
- Use appropriate QoS settings if available
- Monitor for packet loss
- Consider compression for bandwidth-limited scenarios

### 🔍 Debugging
- Start with simple viewers before custom code
- Check Hammerhead configuration files
- Verify port availability and firewall settings
- Use network monitoring tools