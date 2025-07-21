# Point Cloud Soup Recorder

Reconstruct high-resolution point clouds from Hammerhead's `PointCloudSoup` messages, and record as PLY files.

## Installation

```bash
pip install -e examples/python/point_cloud_soup_recorder
```

## Usage

```bash
python point_cloud_soup_recorder.py [hammerhead_ip] [output_directory]
```

### Parameters

- `hammerhead_ip`: IP address of the device running Hammerhead (default: 127.0.0.1)
- `output_directory`: Directory to save PLY files (default: point_clouds folder)

### Examples

```bash
# Record point clouds from local device (default)
python point_cloud_soup_recorder.py

# Record point clouds from local device with custom output directory
python point_cloud_soup_recorder.py 127.0.0.1 /tmp/ply_output

# Record point clouds from remote device with custom output directory
python point_cloud_soup_recorder.py 10.10.1.10 /tmp/ply_output
```

## Output

- **Format**: PLY files (.ply)
- **Location**: `point_clouds` folder (or specified directory)
- **Naming**: Sequential numbering based on frame IDs from messages

## Features

- Subscribe to `PointCloudSoup` messages from Hammerhead
- Reconstruct full point clouds from compact soup representation
- Generate PLY files compatible with CloudCompare and other tools
- Handle high-resolution point clouds efficiently

## `PointCloudSoup` Format

Nodar generates extremely high-resolution point clouds that require efficient network transmission. The `PointCloudSoup` format:

- Provides a compact representation of point cloud data
- Allows reconstruction of full point clouds on client machines
- Reduces network bandwidth requirements significantly
- Maintains high fidelity of 3D spatial information

## Troubleshooting

- **No point clouds generated**: Check IP address and ensure Hammerhead is running
- **Connection hanging**: ZMQ will wait indefinitely for connection - verify network connectivity
- **Large file sizes**: Point clouds are high resolution - ensure adequate storage space

Press `Ctrl+C` to stop recording.