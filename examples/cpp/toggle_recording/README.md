# Toggle Recording

Toggle recording in Hammerhead using ZMQ Request-Reply pattern.

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
./toggle_recording <src_ip>

# Windows
./Release/toggle_recording.exe <src_ip>
```

### Parameters

- `src_ip`: IP address of the ZMQ source (the device running Hammerhead)

### Examples

```bash
# Toggle recording on/off
# Use 127.0.0.1 if running on the same device as Hammerhead
./toggle_recording 127.0.0.1

# Use the network IP address if running on a different device
./toggle_recording 10.10.1.10
```

## Features

- High-performance C++ implementation for real-time recording control
- Request-Reply pattern for reliable recording state changes
- Interactive recording start/stop control
- Immediate response to recording commands
- Minimal latency for recording control

## How It Works

The application uses a Request-Reply pattern to communicate with Hammerhead:

1. Sends recording toggle request to Hammerhead
2. Receives confirmation of recording state change
3. Provides interactive control for starting/stopping recording

The topic names and ports used for this communication are defined in the `topic_ports.hpp` header.

## Troubleshooting

- **No response**: Check IP address and ensure Hammerhead is running
- **Connection hanging**: ZMQ will wait indefinitely for connection - verify network connectivity
- **Recording not toggled**: Ensure Hammerhead is configured to accept recording control commands

Press `Ctrl+C` to stop recording control.