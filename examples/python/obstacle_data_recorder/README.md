# Obstacle Data Recorder

Record obstacle detection data from Hammerhead and save as text files.

## Installation

```bash
pip install -e examples/python/obstacle_data_recorder
```

## Usage

```bash
python obstacle_data_recorder.py <src_ip>
```

### Parameters

- `src_ip`: IP address of the ZMQ source (the device running Hammerhead)

### Examples

```bash
# Record obstacle data from local device
python obstacle_data_recorder.py 127.0.0.1

# Record obstacle data from remote device
python obstacle_data_recorder.py 192.168.1.100
```

## Output

- **Format**: Text files (.txt)
- **Location**: `obstacle_datas` folder
- **Naming**: Sequential numbering with timestamp information

## Data Format

The obstacle data is represented in the XZ plane, where each obstacle is defined by:
- Bounding box coordinates
- Velocity vector (no vertical Y-axis component)

Each generated file contains:
- Header with parameter order information
- Obstacle bounding box data
- Velocity vector data

## Features

- Subscribe to ObstacleData messages from Hammerhead
- Automatic text file generation with headers
- Real-time obstacle data recording
- XZ plane representation for 2D obstacle tracking

## Troubleshooting

- **No data received**: Check IP address and ensure Hammerhead is running
- **Connection hanging**: ZMQ will wait indefinitely for connection - verify network connectivity
- **Empty files**: Ensure Hammerhead is actively detecting obstacles

Press `Ctrl+C` to stop recording.