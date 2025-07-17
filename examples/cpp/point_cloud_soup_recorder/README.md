# Point Cloud Soup Recorder

Reconstruct high-resolution point clouds from Hammerhead's `PointCloudSoup` messages, and record as PLY files.

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Usage

```bash
# Linux
./point_cloud_soup_recorder <src_ip>

# Windows
./Release/point_cloud_soup_recorder.exe <src_ip>
```

### Parameters

- `src_ip`: IP address of the ZMQ source (the device running Hammerhead)

### Examples

```bash
# Record point clouds from local device
./point_cloud_soup_recorder 127.0.0.1

# Record point clouds from remote device
./point_cloud_soup_recorder 192.168.1.100
```

## Output

- **Format**: PLY files (.ply)
- **Location**: `point_clouds` folder
- **Naming**: Sequential numbering based on received messages

## Features

- High-performance C++ implementation for optimal processing speed
- Subscribe to PointCloudSoup messages from Hammerhead
- Reconstruct full point clouds from compact soup representation
- Generate PLY files compatible with CloudCompare and other tools
- Handle high-resolution point clouds efficiently with minimal memory usage

## PointCloudSoup Format

Nodar generates extremely high-resolution point clouds that require efficient network transmission. The PointCloudSoup format:
- Provides a compact representation of point cloud data
- Allows reconstruction of full point clouds on client machines
- Reduces network bandwidth requirements significantly
- Maintains high fidelity of 3D spatial information

The `point_cloud_soup.hpp` header provides the class and functions for reading and writing PointCloudSoup data.

## Troubleshooting

- **No point clouds generated**: Check IP address and ensure Hammerhead is running
- **Connection hanging**: ZMQ will wait indefinitely for connection - verify network connectivity
- **Large file sizes**: Point clouds are high-resolution - ensure adequate storage space

Press `Ctrl+C` to stop recording.