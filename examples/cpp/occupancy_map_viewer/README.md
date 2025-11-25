# Occupancy Map Viewer

Real-time viewer for occupancy grid maps published by Hammerhead's GridDetect feature.

This directory contains two versions:

- **`occupancy_map_viewer`**: Full version with OpenCV visualization (grid overlay, coordinate labels)
- **`occupancy_map_stats`**: Lightweight version without OpenCV (prints metadata and statistics only)

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Both executables will be built in the same directory.

## Usage

### occupancy_map_viewer (with visualization)

```bash
# Linux
./occupancy_map_viewer <src_ip>

# Windows
./Release/occupancy_map_viewer.exe <src_ip>
```

**Requires**: OpenCV 4

### occupancy_map_stats (statistics only)

```bash
# Linux
./occupancy_map_stats <src_ip>

# Windows
./Release/occupancy_map_stats.exe <src_ip>
```

**Requires**: No OpenCV dependency - only ZMQ

### Parameters

- `src_ip`: IP address of the device running Hammerhead

### Examples

```bash
# View occupancy map from local device
./occupancy_map_viewer 127.0.0.1

# Print occupancy statistics from remote device (no OpenCV needed)
./occupancy_map_stats 10.10.1.10
```

## Features

### occupancy_map_viewer
- Binary occupancy grid visualization (white = occupied, black = free)
- Grid overlay with 10-meter spacing in physical coordinates
- Coordinate labels in margins (red for X-axis lateral, green for Z-axis depth)
- Real-time frame statistics: timestamp, grid dimensions, occupied cell count
- Metadata display: grid bounds (xMin, xMax, zMin, zMax) and cell size

### occupancy_map_stats
- Prints frame statistics: frame ID, timestamp, grid dimensions, image type
- Displays occupied cell count and occupancy percentage
- Shows metadata: grid bounds (xMin, xMax, zMin, zMax), cell size, grid dimensions

## Prerequisites

**Important:** Enable GridDetect in Hammerhead configuration:

- Set `enable_grid_detect = 1` in `master_config.ini`
- This automatically enables occupancy map publishing via ZMQ (port 9900)

## Topic Information

| Topic | Port | Description |
|-------|------|-------------|
| `nodar/occupancy_map` | 9900 | Binary occupancy grid (CV_8UC1) with metadata |

## Troubleshooting

- **No display appears**: Check that `enable_grid_detect = 1` in `master_config.ini`
- **Connection timeout**: Verify network connectivity and that Hammerhead is running

Press `Ctrl+C` to exit the viewer.