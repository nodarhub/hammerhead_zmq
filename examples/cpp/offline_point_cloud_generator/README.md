# Offline Point Cloud Generator

Convert saved Hammerhead data into point clouds and save as PLY files.

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
./offline_point_cloud_generator <data_directory> [output_directory]

# Windows
./Release/offline_point_cloud_generator.exe <data_directory> [output_directory]
```

### Parameters

- `data_directory`: Path to directory containing Hammerhead saved data
- `output_directory`: Optional output directory (defaults to `point_clouds` folder in data_directory)

### Examples

```bash
# Generate point clouds with default output location
./offline_point_cloud_generator /path/to/hammerhead/data

# Generate point clouds to specific output directory
./offline_point_cloud_generator /path/to/hammerhead/data /path/to/output
```

## Output

- **Format**: PLY files (.ply)
- **Location**: `point_clouds` folder in data directory (or specified output directory)
- **Naming**: Sequential numbering based on processed data

## Features

- High-performance C++ implementation for fast batch processing
- Convert saved Hammerhead data into full point clouds
- Generate PLY files compatible with CloudCompare and other tools
- Efficient memory usage for large datasets
- Support for both EXR and TIFF depth formats

## Requirements

- OpenCV with EXR support enabled (for legacy EXR files)
- Complete Hammerhead data directory with depth and calibration information

## Troubleshooting

- **EXR support missing**: Install OpenCV with EXR support - available on most Ubuntu x86-64 installations
- **Missing data files**: Ensure data directory contains all required Hammerhead output files
- **ARM compatibility**: Default OpenCV on ARM systems may lack EXR support - use x86-64 system
- **Memory issues**: Large datasets may require significant RAM - monitor system resources

Press `Ctrl+C` to stop generation. 