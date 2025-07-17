# Topbot Publisher

Publish vertically stacked stereo images (topbot format) from disk to Hammerhead.

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

## Output

- **Format**: ZMQ published images
- **Location**: Published to specified port
- **Naming**: Sequential playback of numbered images

## Features

- Publish pre-recorded stereo image pairs to Hammerhead
- Support for vertically stacked stereo image format
- Sequential playback of numbered image sequences
- ZMQ-based communication for real-time streaming

## Topbot Format

The topbot format consists of:
- Left and right stereo images stacked vertically
- Single image file containing both camera views
- Sequential numbering for temporal ordering
- TIFF format for lossless image quality

## Requirements

- Images must be sequentially numbered
- Images must be in TIFF format
- Directory should contain only topbot image files
- Sufficient network bandwidth for real-time streaming

## Troubleshooting

- **Images not found**: Ensure directory contains sequentially numbered TIFF files
- **Connection issues**: Verify port number and network connectivity
- **Playback timing**: Adjust system performance for real-time streaming requirements
- **File format errors**: Ensure all images are valid TIFF files

Press `Ctrl+C` to stop publishing.