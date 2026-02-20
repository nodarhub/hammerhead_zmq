import os
import signal
import sys
import time
from datetime import datetime

import zmq

FRAME_RATE = 10

try:
    from zmq_msgs.navigation import NavigationData
    from zmq_msgs.topic_ports import NAVIGATION_TOPIC
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.navigation_data import NavigationData
    from zmq_msgs.topic_ports import NAVIGATION_TOPIC

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


class NavigationPublisher:
    def __init__(self):
        self.publisher = Publisher(NAVIGATION_TOPIC.name, "", NAVIGATION_TOPIC.port)
        time.sleep(1)

    def publish_navigation(self, nav_data):
        """
        Publish navigation data.

        Args:
            nav_data: NavigationData object
        """
        buffer = bytearray(nav_data.msg_size())
        nav_data.write(buffer, 0)
        self.publisher.send(buffer)
        return True


def main():
    global running

    # Setup signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    publisher = NavigationPublisher()

    print(f"Publishing navigation data at {FRAME_RATE} Hz")
    print("Press Ctrl+C to stop...")

    while running:
        # Get current timestamp
        timestamp = int(datetime.utcnow().timestamp() * 1e9)

        # Create NavigationData message
        nav_data = NavigationData()
        nav_data.timestamp_ns = timestamp

        # IMU data (example values)
        nav_data.imu_timestamp_ns = timestamp
        nav_data.imu_acceleration_x_m_s2 = 0.0
        nav_data.imu_acceleration_y_m_s2 = 0.0
        nav_data.imu_acceleration_z_m_s2 = 9.81
        nav_data.imu_gyro_x_rad_s = 0.0
        nav_data.imu_gyro_y_rad_s = 0.0
        nav_data.imu_gyro_z_rad_s = 0.0
        nav_data.imu_magnetometer_x_gauss = 0.0
        nav_data.imu_magnetometer_y_gauss = 0.0
        nav_data.imu_magnetometer_z_gauss = 0.0
        nav_data.imu_temperature_degC = 25.0

        # GPS data (example values)
        nav_data.gps_timestamp_ns = timestamp
        nav_data.gps_latitude_deg = 0.0
        nav_data.gps_longitude_deg = 0.0
        nav_data.gps_altitude_m = 10.0
        nav_data.gps_horizontal_uncertainty_m = 0.0
        nav_data.gps_vertical_uncertainty_m = 0.0
        nav_data.gps_speed_m_s = 5.0
        nav_data.gps_course_deg = 0.0
        nav_data.gps_fix_type = 0
        nav_data.gps_num_satellites = 1

        # Odometry data (example values - vehicle moving forward at 5 m/s in body frame)
        nav_data.odom_timestamp_ns = timestamp
        nav_data.odom_position_x_m = 0.0
        nav_data.odom_position_y_m = 0.0
        nav_data.odom_position_z_m = 0.0
        nav_data.odom_velocity_x_m_s = 5.0  # 5 m/s forward
        nav_data.odom_velocity_y_m_s = 0.0  # no lateral motion
        nav_data.odom_velocity_z_m_s = 0.0  # no vertical motion
        nav_data.odom_angular_velocity_x_rad_s = 0.0
        nav_data.odom_angular_velocity_y_rad_s = 0.0
        nav_data.odom_angular_velocity_z_rad_s = 0.0

        # Transformation matrix from body frame to Nodar raw camera frame
        # Body frame: x=forward, y=left, z=up
        # Nodar raw camera frame: x=right, y=down, z=forward
        # Transform: Nodar_x = -Body_y, Nodar_y = -Body_z, Nodar_z = Body_x
        nav_data.T_body_to_raw_camera = [
            0.0, -1.0, 0.0, 0.0,  # Row 1: Nodar x = -Body y
            0.0, 0.0, -1.0, 0.0,  # Row 2: Nodar y = -Body z
            1.0, 0.0, 0.0, 0.0,  # Row 3: Nodar z = Body x
            0.0, 0.0, 0.0, 1.0  # Row 4: Homogeneous
        ]

        if publisher.publish_navigation(nav_data):
            print(f"[{timestamp}] Publishing NavigationData:")
            print(
                f"  IMU: accel=({nav_data.imu_acceleration_x_m_s2:.2f}, {nav_data.imu_acceleration_y_m_s2:.2f}, {nav_data.imu_acceleration_z_m_s2:.2f}) m/s², "
                f"gyro=({nav_data.imu_gyro_x_rad_s:.3f}, {nav_data.imu_gyro_y_rad_s:.3f}, {nav_data.imu_gyro_z_rad_s:.3f}) rad/s, "
                f"mag=({nav_data.imu_magnetometer_x_gauss:.2f}, {nav_data.imu_magnetometer_y_gauss:.2f}, {nav_data.imu_magnetometer_z_gauss:.2f}) gauss, "
                f"temp={nav_data.imu_temperature_degC:.1f}°C")
            print(f"  GPS: lat={nav_data.gps_latitude_deg:.6f}°, lon={nav_data.gps_longitude_deg:.6f}°, "
                  f"alt={nav_data.gps_altitude_m:.2f}m, speed={nav_data.gps_speed_m_s:.2f}m/s, "
                  f"fix={nav_data.gps_fix_type}, sats={nav_data.gps_num_satellites}")
            print(
                f"  Odom: pos=({nav_data.odom_position_x_m:.2f}, {nav_data.odom_position_y_m:.2f}, {nav_data.odom_position_z_m:.2f})m, "
                f"vel=({nav_data.odom_velocity_x_m_s:.2f}, {nav_data.odom_velocity_y_m_s:.2f}, {nav_data.odom_velocity_z_m_s:.2f})m/s, "
                f"ang_vel=({nav_data.odom_angular_velocity_x_rad_s:.3f}, {nav_data.odom_angular_velocity_y_rad_s:.3f}, {nav_data.odom_angular_velocity_z_rad_s:.3f})rad/s")
            print()

        time.sleep(1.0 / FRAME_RATE)

    print("\nPublisher stopped.")


if __name__ == "__main__":
    main()
