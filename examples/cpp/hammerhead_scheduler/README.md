# Hammerhead Scheduler

Control Hammerhead's processing schedule to avoid resource conflicts with other intensive processes.

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
./hammerhead_scheduler <src_ip>

# Windows
./Release/hammerhead_scheduler.exe <src_ip>
```

### Parameters

- `src_ip`: IP address of the device running Hammerhead

### Examples

```bash
# Control Hammerhead processing schedule
# Use 127.0.0.1 if running on the same device as Hammerhead
./hammerhead_scheduler 127.0.0.1

# Use the network IP address if running on a different device
./hammerhead_scheduler 10.10.1.10
```

## Configuration Required

In Hammerhead's `master_config.ini` file, set:

```ini
wait_for_scheduler = 1
wait_for_scheduler_timeout_ms = 5000
```

## How It Works

1. Hammerhead processes a frame
2. Sends scheduler request with frame number
3. Waits for scheduler reply before next frame
4. Times out after configured milliseconds if no reply

## Features

- Native C++ performance for low-latency scheduling
- Microsecond-level timing control
- Prevents resource conflicts with other processes
- Configurable timeout protection
- Frame-by-frame processing control

## Troubleshooting

- **No scheduler activity**: Check that `wait_for_scheduler = 1` in `master_config.ini`
- **Timeout errors**: Adjust `wait_for_scheduler_timeout_ms` value
- **Connection issues**: Verify network connectivity and IP address

Press `Ctrl+C` to stop the scheduler.