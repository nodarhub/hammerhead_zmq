# Disparity to Ordered Point Cloud Converter

Convert TIFF disparity images to ordered point clouds.

## Installation

```bash
pip install -e examples/python/disparity_to_ordered_point_cloud
```

## Usage

```bash
cd examples/python/disparity_to_ordered_point_cloud/disparity_to_ordered_point_cloud
python3 disparity_to_ordered_point_cloud.py <disparity_dir> <details_dir> <output_dir> [--split]
```

### Parameters

- `disparity_dir`: Path to directory containing TIFF disparity images
- `details_dir`: Path to directory containing YAML details files
- `output_dir`: Directory where ordered point clouds will be saved
- `--split`: Optional flag to save XYZ dimensions as separate text files

### Examples

```bash
# Generate 3-channel TIFF point clouds (default)
python3 disparity_to_ordered_point_cloud.py /path/to/disparity /path/to/details /path/to/output

# Generate separate text files for each XYZ dimension
python3 disparity_to_ordered_point_cloud.py /path/to/disparity /path/to/details /path/to/output --split
```

## Output

- **Format**: 3-channel TIFF files (default) or separate text files (with --split)
- **Location**: Specified output directory
- **Naming**: Maintains original disparity image naming convention

## Features

- Convert TIFF disparity images to ordered point clouds
- Support for both unified and split output formats
- Preserves spatial ordering of 3D points
- Compatible with various 3D visualization tools

## Output Formats

### Default (3-channel TIFF)
- Single TIFF file per disparity image
- Three channels representing X, Y, Z coordinates
- Maintains pixel-to-point correspondence

### Split Format (--split option)
- Three separate text files per disparity image
- Individual files for X, Y, Z dimensions
- Human-readable format for analysis

## Requirements

- TIFF disparity images from depth_to_disparity converter
- YAML details files from Hammerhead data collection
- Sufficient storage space for point cloud data

## Troubleshooting

- **Missing details files**: Ensure details directory contains corresponding YAML files
- **File overwrite warning**: Existing files in output directory will be overwritten
- **Large output files**: Point clouds can be very large - ensure adequate storage
- **Memory issues**: Processing large disparity images may require significant RAM

Press `Ctrl+C` to stop conversion.