# Point Cloud Recorder

Subscribe to point cloud messages and save them as PLY files for use in CloudCompare and other 3D software.

## Installation

```bash
pip install -e examples/python/point_cloud_recorder
```

## Usage

```bash
# Record standard point clouds
python point_cloud_recorder.py <src_ip>

# Record RGB point clouds (higher bandwidth)
python point_cloud_rgb_recorder.py <src_ip>
```

### Parameters

- `src_ip`: IP address of the device running Hammerhead

### Examples

```bash
# Record point clouds from Hammerhead device
python point_cloud_recorder.py 192.168.1.100

# Record RGB point clouds with color information
python point_cloud_rgb_recorder.py 192.168.1.100
```

## Output

- **Format**: PLY files saved in `point_clouds/` directory
- **Naming**: Sequential numbering (e.g., `cloud_000001.ply`)
- **Compatible with**: CloudCompare, MeshLab, other 3D software

## Features

- Saves point clouds as PLY files
- Support for both standard and RGB point clouds
- Compatible with CloudCompare, MeshLab
- Automatic point cloud validation
- Progress tracking with timestamps

## Bandwidth Warning

**Important**: Point cloud streaming requires significant network bandwidth. RGB point clouds require even more. Use with caution on limited networks.

## Troubleshooting

- **No files created**: Check that Hammerhead is running and point cloud streaming is enabled
- **Connection timeout**: Verify network connectivity and firewall settings
- **High bandwidth usage**: Consider using point cloud soup recorder for reduced bandwidth

Press `Ctrl+C` to stop recording.
