# Depth to Disparity Converter

Convert EXR or TIFF depth images to TIFF disparity images for use with the Nodar Viewer.

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
./depth_to_disparity <data_directory> [output_directory]

# Windows
./Release/depth_to_disparity.exe <data_directory> [output_directory]
```

### Parameters

- `data_directory`: Path to directory containing `depth` and `details` folders from Hammerhead
- `output_directory`: Optional output directory (defaults to `disparity` folder in data_directory)

### Examples

```bash
# Convert depth images with default output location
./depth_to_disparity /path/to/hammerhead/data

# Convert depth images to specific output directory
./depth_to_disparity /path/to/hammerhead/data /path/to/output
```

## Output

- **Format**: TIFF disparity images (.tiff)
- **Location**: `disparity` folder in data directory (or specified output directory)
- **Naming**: Maintains original depth image naming convention

## Features

- High-performance C++ implementation for fast batch processing
- Convert EXR or TIFF depth images to lossless TIFF disparity format
- Compatible with Nodar Viewer for point cloud generation
- Preserves depth information accuracy
- Efficient memory usage for large image datasets

## Requirements

- OpenCV with EXR support enabled
- Both `depth` and `details` folders in input directory
- Details data must be in YAML format

## Troubleshooting

- **EXR support missing**: Install OpenCV with EXR support - available on most Ubuntu x86-64 installations
- **Missing details folder**: Ensure both `depth` and `details` folders exist in data directory
- **File overwrite warning**: Existing files in output directory will be overwritten
- **ARM compatibility**: Default OpenCV on ARM systems may lack EXR support - use x86-64 system

Press `Ctrl+C` to stop conversion.