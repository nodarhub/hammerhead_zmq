# Topbot Publisher

Publish vertically stacked stereo images (which we refer to as `topbot` images) from disk to Hammerhead.

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
./topbot_publisher <topbot_data_directory> <port_number> [pixel_format]

# Windows
./Release/topbot_publisher.exe <topbot_data_directory> <port_number> [pixel_format]
```

### Parameters

- `topbot_data_directory`: Path to directory containing sequentially numbered topbot images
- `port_number`: Port number to publish the images to
- `pixel_format`: Optional pixel format (default: BGR)

### Examples

```bash
# Publish topbot images with default BGR format
./topbot_publisher /path/to/topbot/data 5000

# Publish topbot images with Bayer format
./topbot_publisher /path/to/topbot/data 5000 Bayer_RGGB
```

## Features

- High-performance C++ implementation for real-time streaming
- Publish pre-recorded stereo image pairs to Hammerhead
- Single-pass playback of numbered image sequences (no looping)
- ZMQ-based communication for minimal latency
- Multiple pixel format support

## Supported Pixel Formats

- `BGR` (default)
- `Bayer_RGGB`
- `Bayer_GRBG`
- `Bayer_BGGR`
- `Bayer_GBRG`

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