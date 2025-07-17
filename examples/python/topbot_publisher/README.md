# Topbot Publisher

Publish vertically stacked stereo images (which we refer to as `topbot` images) from disk to Hammerhead.

## Installation

```bash
pip install -e examples/python/topbot_publisher
```

## Usage

```bash
python topbot_publisher <topbot_data_directory> <port_number>
```

### Parameters

- `topbot_data_directory`: Path to directory containing sequentially numbered topbot images
- `port_number`: Port number to publish the images to

### Examples

```bash
# Publish topbot images to port 9800
python topbot_publisher /path/to/topbot/data 9800

# Publish topbot images to custom port
python topbot_publisher /path/to/topbot/data 9850
```

## Features

- Publish pre-recorded stereo image pairs to Hammerhead
- Sequential playback of numbered image sequences
- ZMQ-based communication for real-time streaming

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