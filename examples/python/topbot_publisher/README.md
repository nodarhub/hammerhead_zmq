# Topbot Publisher

Publish vertically stacked stereo images (which we refer to as `topbot` images) from disk to Hammerhead.

## Installation

```bash
pip install -e examples/python/topbot_publisher
```

## Usage

```bash
python topbot_publisher <topbot_data_directory> <port_number> [pixel_format]
```

### Parameters

- `topbot_data_directory`: Path to directory containing sequentially numbered topbot images
- `port_number`: Port number to publish the images to
- `pixel_format`: Optional pixel format (default: BGR)

### Examples

```bash
# Publish topbot images with default BGR format
python topbot_publisher /path/to/topbot/data 5000

# Publish topbot images with Bayer format
python topbot_publisher /path/to/topbot/data 5000 Bayer_RGGB
```

## Supported Pixel Formats

- `BGR` (default)
- `Bayer_RGGB`
- `Bayer_GRBG`
- `Bayer_BGGR`           
- `Bayer_GBRG`

## Features

- Publish pre-recorded stereo image pairs to Hammerhead
- Single-pass playback of numbered image sequences (no looping)
- ZMQ-based communication for real-time streaming
- Multiple pixel format support

## Requirements

- Images must be sequentially numbered
- Images must be in TIFF format
- Directory should contain only topbot image files
- Sufficient network bandwidth for real-time streaming

## Troubleshooting

- **Images not found**: Ensure the directory contains sequentially numbered TIFF files
- **Connection issues**: Verify port number and network connectivity
- **File format errors**: Ensure all images are valid TIFF files

Press `Ctrl+C` to stop publishing.