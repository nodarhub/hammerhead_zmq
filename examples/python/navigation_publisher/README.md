# Navigation Publisher

Publish navigation data (IMU, GPS, Odometry) with coordinate system transformations to Hammerhead.

## Installation

```bash
pip install -e examples/python/navigation_publisher
```

## Configuration

Before running the navigation publisher, ensure the following settings are configured in `master_config.ini`:

```ini
# [bool] Enable external navigation subscriber via ZMQ.
# This allows hammerhead to receive navigation data from an external publisher.
enable_navigation_subscriber = 1

# [string] IP address of the navigation publisher.
# Only used if enable_navigation_subscriber is enabled.
navigation_publisher_ip = 127.0.0.1
```

**Note**: Set `navigation_publisher_ip` to the IP address where the navigation publisher is running. Use `127.0.0.1` for local testing.

### Examples

```bash
# Run the navigation publisher
python navigation_publisher
```

The publisher will continuously send navigation data at 10 Hz with example values:

- **IMU**: Acceleration (0, 0, 9.81) m/s², Gyro (0, 0, 0) rad/s, Magnetometer (0, 0, 0) gauss, Temperature 25°C
- **GPS**: Lat 0.0°, Lon 0.0°, Alt 10.0m, Speed 5.0 m/s, Fix type 0, 1 satellite
- **Odometry**: Position (0, 0, 0)m, Velocity (5, 0, 0) m/s, Angular velocity (0, 0, 0) rad/s

## Features

- Python implementation for easy integration and customization
- Publishes comprehensive navigation data including IMU, GPS, and Odometry
- Individual timestamps for each sensor (IMU, GPS, Odometry)
- Includes 4x4 transformation matrix from body frame to Nodar raw camera frame
- ZMQ-based communication on port 9824 (topic: `nodar/navigation`)
- Publishes at 10 Hz update rate

## Coordinate Systems

The body/odometry frame used in this example is:

### Body/Odometry Frame (Example)
- **x**: Forward direction
- **y**: Left direction
- **z**: Up direction

**Note**: You can use any body frame coordinate system. Simply adjust the `T_body_to_raw_camera` transformation matrix accordingly to match your coordinate system.

### Nodar Raw Camera Frame
- **x**: Right direction
- **y**: Down direction
- **z**: Forward direction

## Message Format

The NavigationData message contains:

- **Timestamp**: 64-bit nanosecond timestamp (message creation time)

- **IMU Data**:
    - Timestamp (ns)
    - Acceleration (x, y, z) in m/s²
    - Gyroscope (x, y, z) in rad/s
    - Magnetometer (x, y, z) in gauss
    - Temperature in °C

- **GPS Data**:
    - Timestamp (ns)
    - Latitude, Longitude (degrees), Altitude (m)
    - Horizontal/Vertical uncertainty (m)
    - Speed (m/s), Course (degrees)
    - Fix type, Number of satellites

- **Odometry Data**:
    - Timestamp (ns)
    - Position (x, y, z) in m
    - Velocity (x, y, z) in m/s
    - Angular velocity (x, y, z) in rad/s

- **Transformation Matrix**: 4x4 matrix from body to raw camera frame (row-major)

## Use Cases

This example is useful for:

- Testing navigation data integration in perception systems
- Validating coordinate system transformations
- Providing multi-sensor navigation data (IMU, GPS, Odometry) to Hammerhead
- Sensor fusion and localization testing

## Customization

To modify the navigation values, edit the field assignments in `navigation_publisher.py`:

```python
# IMU data (example values)
nav_data.imu_acceleration_z_m_s2 = 9.81  # Gravity

# GPS data (example values)
nav_data.gps_altitude_m = 10.0

# Odometry data (example values)
nav_data.odom_velocity_x_m_s = 5.0  # 5 m/s forward
```

To change the transformation matrix, modify the `T_body_to_raw_camera` list accordingly.

## Troubleshooting

- **Port conflict**: Ensure port 9824 is not in use by another application
- **Hammerhead not receiving data**: Verify `enable_navigation_subscriber = 1` and `navigation_publisher_ip` is correctly set in `master_config.ini`

Press `Ctrl+C` to stop publishing.