# Image Viewer

Real-time OpenCV viewer for stereo images, disparity maps, and depth data published by Hammerhead.

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
./image_viewer <src_ip> <image_topic_or_port>

# Windows
./Release/image_viewer.exe <src_ip> <image_topic_or_port>
```

### Parameters

- `src_ip`: IP address of the device running Hammerhead
- `image_topic_or_port`: Topic name or port number (see Available Topics below)

### Examples

```bash
# View raw left camera
# Use 127.0.0.1 if running on the same device as Hammerhead
./image_viewer 127.0.0.1 nodar/left/image_raw

# Use the network IP address if running on a different device
./image_viewer 192.168.1.100 nodar/left/image_raw

# View disparity map
./image_viewer 192.168.1.100 9804

# View color-blended depth
./image_viewer 192.168.1.100 nodar/color_blended_depth/image_raw
```

## Available Camera Topics

| Topic | Port | Description |
|-------|------|-------------|
| `nodar/left/image_raw` | 9800 | Raw left camera |
| `nodar/right/image_raw` | 9801 | Raw right camera |
| `nodar/left/image_rect` | 9802 | Rectified left image |
| `nodar/right/image_rect` | 9803 | Rectified right image |
| `nodar/disparity` | 9804 | Disparity map |
| `nodar/color_blended_depth/image_raw` | 9805 | Color-coded depth visualization |

## Features

- Optimized for real-time performance
- Support for all image topic types

## Troubleshooting

- **No display appears**: Check that Hammerhead is running and the IP address is correct
- **Connection timeout**: Verify network connectivity and firewall settings
- **Invalid topic**: Use one of the topics listed in the Available Topics section

Press `Ctrl+C` to exit the viewer.
