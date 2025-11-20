# Occupancy Map Viewer

Real-time viewer for occupancy grid maps with coordinate overlay, published by Hammerhead's GridDetect feature.

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
./occupancy_map_viewer <src_ip>

# Windows
./Release/occupancy_map_viewer.exe <src_ip>
```

### Parameters

- `src_ip`: IP address of the device running Hammerhead

### Examples

```bash
# View occupancy map from local device
./occupancy_map_viewer 127.0.0.1

# View occupancy map from remote device
./occupancy_map_viewer 10.10.1.10
```

## Features

- Binary occupancy grid visualization (white = occupied, black = free)
- Grid overlay with 10-meter spacing in physical coordinates
- Coordinate labels in margins (red for X-axis lateral, green for Z-axis depth)
- Real-time frame statistics: timestamp, grid dimensions, occupied cell count
- Metadata display: grid bounds (xMin, xMax, zMin, zMax) and cell size

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