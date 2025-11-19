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
./point_cloud_soup_recorder [OPTIONS] [hammerhead_ip] [output_directory]

# Windows
./Release/point_cloud_soup_recorder.exe [OPTIONS] [hammerhead_ip] [output_directory]
```

### Options

- `-w`, `--wait-for-scheduler`: Enable scheduler synchronization with Hammerhead. When enabled, the recorder will wait for Hammerhead to request the next frame before continuing, ensuring frame-by-frame synchronization.
- `-h`, `--help`: Display usage information

### Parameters

- `hammerhead_ip`: IP address of the device running Hammerhead (default: 127.0.0.1)
- `output_directory`: Directory to save PLY files (default: point_clouds folder)

### Examples

```bash
# Record point clouds from local device (default)
./point_cloud_soup_recorder

# Record point clouds from local device with custom output directory
./point_cloud_soup_recorder 127.0.0.1 /tmp/ply_output

# Record point clouds from remote device with custom output directory
./point_cloud_soup_recorder 10.10.1.10 /tmp/ply_output

# Record with scheduler synchronization (frame-by-frame)
./point_cloud_soup_recorder -w 10.10.1.10 /tmp/ply_output
./point_cloud_soup_recorder --wait-for-scheduler 10.10.1.10 /tmp/ply_output

```

### Scheduler Synchronization Workflow

When using the `-w` flag for frame-by-frame synchronization:

1. **Configure Hammerhead** - Edit `master_config.ini`:
   ```ini
   wait_for_scheduler = 1
   ```

2. **Start the recorder first**:
   ```bash
   ./point_cloud_soup_recorder -w
   ```

3. **Start Hammerhead** - The recorder will now control frame processing timing

This ensures proper ZMQ connection establishment.

## Output

- **Format**: PLY files (.ply)
- **Location**: `point_clouds` folder
- **Naming**: Sequential numbering based on received messages

## Features

- High-performance C++ implementation for optimal processing speed
- Subscribe to `PointCloudSoup` messages from Hammerhead
- Reconstruct full point clouds from compact soup representation
- Generate PLY files compatible with CloudCompare and other tools
- Handle high-resolution point clouds efficiently with minimal memory usage
- Optional scheduler synchronization for frame-by-frame processing with Hammerhead

## `PointCloudSoup` Format

Nodar generates extremely high-resolution point clouds that require efficient network transmission. The `PointCloudSoup` format:

- Provides a compact representation of point cloud data
- Allows reconstruction of full point clouds on client machines
- Reduces network bandwidth requirements significantly
- Maintains high fidelity of 3D spatial information

The `point_cloud_soup.hpp` header provides the class and functions for reading and writing `PointCloudSoup` data.

## Troubleshooting

- **No point clouds generated**: Check IP address and ensure Hammerhead is running
- **Connection hanging**: ZMQ will wait indefinitely for connection - verify network connectivity
- **Large file sizes**: Point clouds are high resolution - ensure adequate storage space

Press `Ctrl+C` to stop recording.