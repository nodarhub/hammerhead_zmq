# Depth to Disparity Converter

Convert EXR depth images to TIFF disparity images for use with the Nodar Viewer.

## Installation

```bash
pip install -e examples/python/depth_to_disparity
```

## Usage

```bash
cd examples/python/depth_to_disparity/depth_to_disparity
OPENCV_IO_ENABLE_OPENEXR=1 python3 depth_to_disparity <data_directory> [output_directory]
```

### Parameters

- `data_directory`: Path to directory containing `depth` and `details` folders from Hammerhead
- `output_directory`: Optional output directory (defaults to `disparity` folder in data_directory)

### Examples

```bash
# Convert depth images with default output location
OPENCV_IO_ENABLE_OPENEXR=1 python3 depth_to_disparity /path/to/hammerhead/data

# Convert depth images to specific output directory
OPENCV_IO_ENABLE_OPENEXR=1 python3 depth_to_disparity /path/to/hammerhead/data /path/to/output
```

## Output

- **Format**: TIFF disparity images (.tiff)
- **Location**: `disparity` folder in data directory (or specified output directory)
- **Naming**: Maintains original depth image naming convention

## Features

- Convert EXR depth images to lossless TIFF disparity format
- Compatible with Nodar Viewer for point cloud generation
- Preserves depth information accuracy
- Batch processing of entire directories

## Requirements

- OpenCV with EXR support enabled
- Both `depth` and `details` folders in input directory
- Details data must be in YAML format

## Troubleshooting

- **EXR support missing**: Install OpenCV with EXR support or use `OPENCV_IO_ENABLE_OPENEXR=1`
- **Missing details folder**: Ensure both `depth` and `details` folders exist in data directory
- **File overwrite warning**: Existing files in output directory will be overwritten
- **Orin compatibility**: Default OpenCV on Orin may lack EXR support - use x86-64 system

Press `Ctrl+C` to stop conversion.