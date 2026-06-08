# Point Cloud Soup Recorder

Record Hammerhead `PointCloudSoup` messages to disk. Two output formats are supported:

- **`ply` (default)** — reconstruct a 3D point cloud on the recording host and write one
  `{frame_id}.ply` per message, alongside a `details/{frame_id}.yaml` that preserves the
  timestamps, baseline, focal length, and projection matrices carried by the soup.
- **`raw`** — skip reconstruction. Write the disparity and left rectified images plus the
  `details/{frame_id}.yaml` into three sibling folders (`disparity/`, `left-rect/`, `details/`).
  Reconstruction can be deferred to an offline step, keeping CPU usage low on the
  recording host.

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
./point_cloud_soup_recorder [OPTIONS]

# Windows
./Release/point_cloud_soup_recorder.exe [OPTIONS]
```

### Options

- `--ip ADDR` — IP address of the device running Hammerhead (default: `127.0.0.1`).
- `--output-dir DIR` — base directory to write output into (default: current working
  directory). A UTC-timestamped subfolder (`YYYYMMDD-HHMMSS`) is appended so each run
  lands in its own directory — matching the convention used by the `image_recorder`
  example.
- `-f`, `--format {ply,raw}` — output format (default: `ply`).
- `--downsample N` — *[ply format only]* keep every Nth valid point when writing the PLY.
  Default: `1` (keep all points); increase if you see dropped frames. See
  [Downsampling](#downsampling) below.
- `-w`, `--wait-for-scheduler` — enable scheduler synchronization with Hammerhead. When
  enabled, the recorder waits for Hammerhead to request the next frame before continuing,
  ensuring frame-by-frame synchronization.
- `-h`, `--help` — show the full help text.

### Examples

```bash
# Record PLY point clouds (every point kept by default)
./point_cloud_soup_recorder --ip 10.10.1.10 --output-dir /tmp/ply_output

# Record PLY keeping every 10th point (e.g. if disk can't keep up at full rate)
./point_cloud_soup_recorder --downsample 10

# Record raw disparity + left-rect + details for offline reconstruction
./point_cloud_soup_recorder --ip 10.10.1.10 --format raw --output-dir /tmp/recording

# Record with scheduler synchronization (frame-by-frame)
./point_cloud_soup_recorder --ip 10.10.1.10 -w
```

### Scheduler synchronization workflow

When using the `-w` flag for frame-by-frame synchronization:

1. **Configure Hammerhead** — edit `master_config.ini`:
   ```ini
   wait_for_scheduler = 1
   ```

2. **Start the recorder first**:
   ```bash
   ./point_cloud_soup_recorder -w
   ```

3. **Start Hammerhead** — the recorder will now control frame processing timing.

This ensures proper ZMQ connection establishment.

## Output

Each run writes into `{output_directory}/{YYYYMMDD-HHMMSS}/`.

### `ply` format

```
{output_directory}/{YYYYMMDD-HHMMSS}/
  point_clouds/
    000000000.ply
    000000001.ply
    ...
  details/
    000000000.yaml
    000000001.yaml
    ...
```

### `raw` format

```
{output_directory}/{YYYYMMDD-HHMMSS}/
  disparity/
    000000000.tiff     # no compression
    ...
  left-rect/
    000000000.tiff     # no compression
    ...
  details/
    000000000.yaml
    ...
```

Filenames are 9-digit zero-padded frame IDs. The `details/{frame_id}.yaml` captures the
non-image fields carried by the soup (timestamp, baseline, focal length, projection and
rotation matrices).

## Downsampling

In `ply` mode, every Nth valid point is kept (`--downsample N`, default `N=1` — every
point is kept).

Writing every point of a high-resolution point cloud to PLY can outpace disk bandwidth
at high frame rates, and dropped frames will result. If you see dropped frames, either
increase `N`, or switch to `--format raw` and reconstruct offline.

Downsampling has no effect in `raw` mode.

## `PointCloudSoup` format

Nodar's point clouds are transmitted in a compact form to reduce network bandwidth.
Clients can either reconstruct a full 3D point cloud from each message (what `--format ply`
does on the recording host) or record the underlying images directly (what `--format raw`
does) and reconstruct offline. Both paths preserve the metadata needed for reconstruction.

The `point_cloud_soup.hpp` header provides the class and functions for reading and writing
`PointCloudSoup` data.

## Troubleshooting

- **No point clouds generated** — check the IP address and ensure Hammerhead is running.
- **Connection hanging** — ZMQ will wait indefinitely for a connection; verify network
  connectivity.
- **Dropped frames** — increase `--downsample`, or switch to `--format raw`.
- **Large file sizes** — point clouds are high resolution; ensure adequate storage.

Press `Ctrl+C` to stop recording.
