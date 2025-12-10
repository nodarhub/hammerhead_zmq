import os
import signal
import sys
import time
from datetime import datetime

import zmq

FRAME_RATE = 10

try:
    from zmq_msgs.velocity import VelocityData
    from zmq_msgs.topic_ports import VELOCITY_TOPIC
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.velocity import VelocityData
    from zmq_msgs.topic_ports import VELOCITY_TOPIC

# Global variable for signal handling
running = True


def signal_handler(signum, frame):
    global running
    print("\nSIGINT or SIGTERM received. Exiting...", file=sys.stderr)
    running = False


class Publisher:
    def __init__(self, topic, ip, port):
        self.context = zmq.Context(1)
        self.socket = self.context.socket(zmq.PUB)
        self.socket.setsockopt(zmq.SNDHWM, 1)
        if ip:
            self.socket.connect(f"tcp://{ip}:{port}")
        else:
            self.socket.bind(f"tcp://*:{port}")

    def send(self, buffer):
        self.socket.send(buffer)


class VelocityPublisher:
    def __init__(self):
        self.publisher = Publisher(VELOCITY_TOPIC.name, "", VELOCITY_TOPIC.port)
        time.sleep(1)

    def publish_velocity(self, timestamp, velocity, rotation_to_nodar_raw):
        """
        Publish velocity data.

        Args:
            timestamp: timestamp in nanoseconds
            velocity: list/tuple of 3 floats [vx, vy, vz] in m/s
            rotation_to_nodar_raw: list/tuple of 9 floats representing 3x3 rotation matrix
        """
        velocity_data = VelocityData(timestamp, velocity, rotation_to_nodar_raw)
        buffer = bytearray(velocity_data.msg_size())
        velocity_data.write(buffer, 0)
        self.publisher.send(buffer)
        return True


def main():
    global running

    # Setup signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    publisher = VelocityPublisher()

    print(f"Publishing velocity data at {FRAME_RATE} Hz")
    print("Press Ctrl+C to stop...")

    while running:
        # Get current timestamp
        timestamp = int(datetime.now().timestamp() * 1e9)

        # Example velocity data in odometry coordinate system
        # Odometry system: x=forward, y=left, z=up
        velocity = [
            5.0,  # vx: 5 m/s forward
            0.0,  # vy: no lateral motion
            0.0   # vz: no vertical motion
        ]

        # Rotation matrix from odometry to Nodar raw camera coordinate system
        # Nodar raw camera system: x=right, y=down, z=forward
        # Transform: Nodar_x = -Odom_y, Nodar_y = -Odom_z, Nodar_z = Odom_x
        rotation_to_nodar_raw = [
            0.0, -1.0, 0.0,  # Row 1: Nodar x = -Odom y
            0.0, 0.0, -1.0,  # Row 2: Nodar y = -Odom z
            1.0, 0.0, 0.0    # Row 3: Nodar z = Odom x
        ]

        if publisher.publish_velocity(timestamp, velocity, rotation_to_nodar_raw):
            print(
                f"\rPublishing | vx: {velocity[0]:.2f} m/s, vy: {velocity[1]:.2f} m/s, vz: {velocity[2]:.2f} m/s",
                end="",
                flush=True
            )

        time.sleep(1.0 / FRAME_RATE)

    print("\nPublisher stopped.")


if __name__ == "__main__":
    main()