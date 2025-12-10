# Velocity Publisher

Publish velocity data with coordinate system transformations to Hammerhead.

## Installation

```bash
pip install -e examples/python/velocity_publisher
```

## Configuration

Before running the velocity publisher, ensure the following settings are configured in `master_config.ini`:

```ini
# [bool] Enable external velocity subscriber via ZMQ.
# This allows hammerhead to receive velocity data from an external publisher.
enable_velocity_subscriber = 1

# [string] IP address of the velocity publisher.
# Only used if enable_velocity_subscriber is enabled.
velocity_publisher_ip = 127.0.0.1
```

**Note**: Set `velocity_publisher_ip` to the IP address where the velocity publisher is running. Use `127.0.0.1` for local testing.

### Examples

```bash
# Run the velocity publisher
python velocity_publisher
```

The publisher will continuously send velocity data at 10 Hz with example values:

- Forward velocity: 5.0 m/s
- Lateral velocity: 0.0 m/s
- Vertical velocity: 0.0 m/s

## Features

- Python implementation for easy integration and customization
- Publishes velocity data in odometry coordinate system
- Includes rotation matrix for coordinate system transformation
- ZMQ-based communication on port 9824 (topic: `nodar/velocity`)
- Publishes at 10 Hz update rate

## Coordinate Systems assumed in the example

### Odometry Coordinate System
- **x**: Forward direction
- **y**: Left direction
- **z**: Up direction

### Nodar Raw Camera Coordinate System
- **x**: Right direction
- **y**: Down direction
- **z**: Forward direction

## Message Format

The velocity message contains:

- **Timestamp**: 64-bit nanosecond timestamp
- **Velocity**: 3 floats [vx, vy, vz] in m/s
- **Rotation Matrix**: 9 floats representing a 3x3 rotation matrix (row-major)

## Use Cases

This example is useful for:

- Testing velocity integration in perception systems
- Validating coordinate system transformations
- Providing ego-motion data to obstacle detection

## Customization

To modify the velocity values, edit the `velocity` list in `velocity_publisher.py`:

```python
velocity = [
    5.0,  # vx: forward velocity in m/s
    0.0,  # vy: lateral velocity in m/s
    0.0   # vz: vertical velocity in m/s
]
```

To change the rotation matrix, modify the `rotation_to_nodar_raw` list accordingly.

## Troubleshooting

- **Port conflict**: Ensure port 9824 is not in use by another application
- **Hammerhead not receiving data**: Verify `enable_velocity_subscriber = 1` and `velocity_publisher_ip` is correctly set in `master_config.ini`

Press `Ctrl+C` to stop publishing.