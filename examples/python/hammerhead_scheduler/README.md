# Hammerhead Scheduler

Control Hammerhead's processing schedule to avoid resource conflicts with other intensive processes.

## Installation

```bash
pip install -e examples/python/hammerhead_scheduler
```

## Usage

```bash
python hammerhead_scheduler.py <src_ip>
```

### Parameters

- `src_ip`: IP address of the device running Hammerhead

### Examples

```bash
# Control Hammerhead processing schedule
python hammerhead_scheduler.py 192.168.1.100
```

## Configuration Required

In Hammerhead's `master_config.ini` file, set:

```ini
wait_for_scheduler = true
wait_for_scheduler_timeout_ms = 5000
```

## How It Works

1. Hammerhead processes a frame
2. Sends scheduler request with frame number
3. Waits for scheduler reply before next frame
4. Times out after configured milliseconds if no reply

## Features

- Microsecond-level timing control
- Prevents resource conflicts with other processes
- Configurable timeout protection
- Frame-by-frame processing control

## Use Cases

- Coordinating with other intensive processes
- Custom frame rate control
- Synchronization with external systems
- Resource management on constrained systems

## Troubleshooting

- **No scheduler activity**: Check that `wait_for_scheduler = true` in `master_config.ini`
- **Timeout errors**: Adjust `wait_for_scheduler_timeout_ms` value
- **Connection issues**: Verify network connectivity and IP address

Press `Ctrl+C` to stop the scheduler.