# Image Viewer

Real-time OpenCV viewer for stereo images, disparity maps, and depth data published by Hammerhead.

## Installation

```bash
pip install -e examples/python/image_viewer
```

## Usage

```bash
python image_viewer.py <src_ip> <image_topic_or_port>
```

### Parameters

- `src_ip`: IP address of the device running Hammerhead
- `image_topic_or_port`: Topic name or port number (see Available Topics below)

### Examples

```bash
# View raw left camera
# Use 127.0.0.1 if running on the same device as Hammerhead
python image_viewer.py 127.0.0.1 nodar/left/image_raw

# Use the network IP address if running on a different device
python image_viewer.py 10.10.1.10 nodar/left/image_raw

# View disparity map
python image_viewer.py 10.10.1.10 9804

# View color-blended depth
python image_viewer.py 10.10.1.10 nodar/color_blended_depth/image_raw
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
| `nodar/topbot_raw` | 9813 | Raw top (left) and bottom (right) camera pair |
| `nodar/topbot_rect` | 9823 | Rectified top (left) and bottom (right) camera pair |
| `nodar/occupancy_map` | 9900 | Occupancy map |

## Features

- Support for all image topics (raw, rectified, disparity, depth)
- Real-time display with OpenCV

## Troubleshooting

- **No display appears**: Check that Hammerhead is running and the IP address is correct
- **Connection timeout**: Verify network connectivity and firewall settings
- **Invalid topic**: Use one of the topics listed in the Available Topics section

Press `Ctrl+C` to exit the viewer.
