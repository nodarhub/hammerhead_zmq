# Image Recorder

Record images from Hammerhead with ZMQ.

## Installation

```bash
pip install -e examples/python/image_recorder
```

## Usage

```bash
python image_recorder.py <src_ip> <image_topic_or_port> <output_dir>
```

### Parameters

- `src_ip`: IP address of the ZMQ source (the device running Hammerhead)
- `image_topic_or_port`: Topic name or port number for the image stream
- `output_dir`: Folder where the images will be saved

### Examples

```bash
# Record raw right images using port number
python image_recorder.py 127.0.0.1 9801 raw_right_images

# Record raw right images using topic name
python image_recorder.py 127.0.0.1 nodar/right/image_raw output_dir
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

## Output

The recorder creates a timestamped folder structure:

```
<output_dir>/YYYYMMDD-HHMMSS/
├── <topic_name>/           # Image subdirectory (e.g., left_raw/, topbot_raw/)
│   ├── 000000001.tiff
│   ├── 000000002.tiff
│   └── ...
├── times/                  # Individual timestamp files
│   ├── 000000001.txt
│   ├── 000000002.txt
│   └── ...
├── times.txt               # Consolidated timestamps (one line per frame)
└── loop_latency.txt        # Performance metrics
```

- **Images**: TIFF format with no compression
- **Naming**: 9-digit zero-padded frame numbers
- **Timestamps**: Each frame's timestamp(s) saved in `times/` and consolidated in `times.txt`
- **TOPBOT metadata**: For topbot images, dual timestamps (left/right) are embedded in TIFF metadata

## Features

- Subscribe to any image topic published by Hammerhead
- Support for both topic names and port numbers
- Real-time recording with minimal latency

## Troubleshooting

- **No images received**: Check IP address and ensure Hammerhead is running
- **Invalid topic**: Verify topic name exists in `topic_ports.py`
- **Connection hanging**: ZMQ will wait indefinitely for connection - check network connectivity

Press `Ctrl+C` to stop recording.