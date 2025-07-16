# Legacy Obstacle Data Converter

Convert legacy `bounding_boxes_and_velocities` data to `tracked-objects` format for use with the Nodar Viewer.

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
./legacy_obstacle_data_converter <data_directory> [output_directory]

# Windows
./Release/legacy_obstacle_data_converter.exe <data_directory> [output_directory]
```

### Parameters

- `data_directory`: Path to directory containing `bounding_boxes_and_velocities` folder from Hammerhead
- `output_directory`: Optional output directory (defaults to `tracked-objects` folder in data_directory)

### Examples

```bash
# Convert legacy data with default output location
./legacy_obstacle_data_converter /path/to/hammerhead/data

# Convert legacy data to specific output directory
./legacy_obstacle_data_converter /path/to/hammerhead/data /path/to/output
```

## Output

- **Format**: Tracked-objects data files
- **Location**: `tracked-objects` folder in data directory (or specified output directory)
- **Naming**: Maintains original data naming convention

## Features

- High-performance C++ implementation for fast batch conversion
- Convert legacy `bounding_boxes_and_velocities` data to modern `tracked-objects` format
- Compatible with Nodar Viewer for displaying object bounding boxes
- Preserves object tracking information and velocity data
- Efficient processing of large datasets

## Use Case

This converter is needed when working with older Hammerhead data that used the legacy `bounding_boxes_and_velocities` format. The converted `tracked-objects` data can be displayed in the Nodar Viewer for visualization of detected objects with their bounding boxes and tracking information.

## Requirements

- Data directory must contain `bounding_boxes_and_velocities` folder
- Legacy data must be in the expected format from older Hammerhead versions

## Troubleshooting

- **Missing legacy folder**: Ensure `bounding_boxes_and_velocities` folder exists in data directory
- **File overwrite warning**: Existing files in output directory will be overwritten
- **Format errors**: Ensure legacy data is in the expected format from older Hammerhead versions

Press `Ctrl+C` to stop conversion.