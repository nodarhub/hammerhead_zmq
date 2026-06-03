# Topbot Metadata Writer

Write details YAML metadata into the TIFF Software tag (305) of topbot TIFF files.

## Installation

```bash
pip install -e examples/python/topbot_metadata_writer
```

## Usage

```bash
topbot_metadata_writer <folder>
```

### Parameters

- `folder`: Path to a folder containing `topbot/` and `details/` subdirectories

### Expected folder structure

```
folder/
  topbot/
    000000.tiff
    000001.tiff
    ...
  details/
    000000.yaml
    000001.yaml
    ...
```

Each YAML file in `details/` is matched to the TIFF file in `topbot/` with the same stem name. TIFF files without a matching YAML are skipped.

### Example

```bash
topbot_metadata_writer /path/to/recording
```

## How it works

For each TIFF file in `topbot/`, the tool:

1. Reads the matching YAML file from `details/`
2. Formats the YAML data into a `DETAILS: "..."` string
3. Injects it as the TIFF Software tag (305) in every IFD
4. Writes the updated TIFF back in place