# Point Cloud Recorder

Subscribe to point cloud messages and save them as PLY files for use in CloudCompare and other 3D software.

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Usage

```bash
# Linux - Record standard point clouds
./point_cloud_recorder <src_ip>

# Linux - Record RGB point clouds (higher bandwidth)
./point_cloud_rgb_recorder <src_ip>

# Windows - Record standard point clouds
./Release/point_cloud_recorder.exe <src_ip>

# Windows - Record RGB point clouds (higher bandwidth)
./Release/point_cloud_rgb_recorder.exe <src_ip>
```

### Parameters

- `src_ip`: IP address of the device running Hammerhead

### Examples

```bash
# Record point clouds from Hammerhead device
./point_cloud_recorder 192.168.1.100

# Record RGB point clouds with color information
./point_cloud_rgb_recorder 192.168.1.100
```

## Output

- **Format**: PLY files saved in `point_clouds/` directory
- **Naming**: Sequential numbering (e.g., `cloud_000001.ply`)
- **Compatible with**: CloudCompare, MeshLab, other 3D software

## Features

- Optimized memory allocation
- Fast PLY file writing
- Support for RGB point clouds
- Real-time performance monitoring
- Progress tracking with timestamps

## Bandwidth Warning

**Important**: Point cloud streaming requires significant network bandwidth. RGB point clouds require even more. Use with caution on limited networks.

## Troubleshooting

- **No files created**: Check that Hammerhead is running and point cloud streaming is enabled
- **Connection timeout**: Verify network connectivity and firewall settings
- **High bandwidth usage**: Consider using point cloud soup recorder for reduced bandwidth

Press `Ctrl+C` to stop recording.
