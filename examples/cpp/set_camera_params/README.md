# Set Camera Parameters

Modify camera parameters in Hammerhead using ZMQ Request-Reply pattern.

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
./set_exposure <src_ip>
./set_gain <src_ip>

# Windows
./Release/set_exposure.exe <src_ip>
./Release/set_gain.exe <src_ip>
```

### Parameters

- `src_ip`: IP address of the ZMQ source (the device running Hammerhead)

### Examples

```bash
# Set camera exposure
./set_exposure 192.168.1.100

# Set camera gain
./set_gain 192.168.1.100
```

## Output

This example provides interactive control for:
- Camera exposure settings
- Camera gain settings

## Features

- High-performance C++ implementation for real-time parameter control
- Request-Reply pattern for reliable parameter setting
- Interactive exposure control
- Interactive gain control
- Real-time parameter adjustment

## Available Controls

- **Exposure**: Adjust camera exposure settings
- **Gain**: Modify camera gain parameters

The topic names and ports used for this communication are defined in the `topic_ports.hpp` header.

## Troubleshooting

- **No response**: Check IP address and ensure Hammerhead is running
- **Connection hanging**: ZMQ will wait indefinitely for connection - verify network connectivity
- **Parameter not applied**: Ensure Hammerhead is configured to accept parameter changes

Press `Ctrl+C` to stop parameter control.