# Image Recorder

Record images from Hammerhead with ZMQ.

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
./image_recorder <src_ip> <image_topic_or_port> <output_dir>

# Windows
./Release/image_recorder.exe <src_ip> <image_topic_or_port> <output_dir>
```

### Parameters

- `src_ip`: IP address of the ZMQ source (the device running Hammerhead)
- `image_topic_or_port`: Topic name or port number for the image stream
- `output_dir`: Folder where the images will be saved

### Examples

```bash
# Record raw right images using port number
./image_recorder 127.0.0.1 9801 raw_right_images

# Record raw right images using topic name
./image_recorder 127.0.0.1 nodar/right/image_raw raw_right_images
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

## Output

- **Format**: Image files
- **Location**: Specified output directory
- **Naming**: Sequential numbering based on received images

## Features

- High-performance C++ implementation for minimal latency
- Subscribe to any image topic published by Hammerhead
- Support for both topic names and port numbers
- Automatic image format handling
- Real-time recording with optimized memory usage

## Valid Image Topics

- `nodar/color_blended_depth/image_raw`
- `nodar/disparity`
- `nodar/left/image_raw`
- `nodar/left/image_rect`
- `nodar/right/image_raw`
- `nodar/right/image_rect`

## Topic to Port Mapping

The file `topic_ports.hpp` defines the `topic->port` mappings. For example:
- `"nodar/right/image_raw"` maps to port `9801`

## Troubleshooting

- **No images received**: Check IP address and ensure Hammerhead is running
- **Invalid topic**: Verify topic name exists in `topic_ports.hpp`
- **Connection hanging**: ZMQ will wait indefinitely for connection - check network connectivity

Press `Ctrl+C` to stop recording.
