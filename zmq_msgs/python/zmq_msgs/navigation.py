import struct

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


class NavigationData:
    """
    Message for navigation data including IMU, GPS, and Odometry.

    All data is in the body/odometry frame unless otherwise specified.
    T_body_to_raw_camera transforms from body frame to Nodar raw camera frame.
    """

    def __init__(self):
        """Initialize NavigationData with default values."""
        self.timestamp_ns = 0

        # IMU data
        self.imu_timestamp_ns = 0
        self.imu_acceleration_x_m_s2 = 0.0
        self.imu_acceleration_y_m_s2 = 0.0
        self.imu_acceleration_z_m_s2 = 0.0
        self.imu_gyro_x_rad_s = 0.0
        self.imu_gyro_y_rad_s = 0.0
        self.imu_gyro_z_rad_s = 0.0
        self.imu_magnetometer_x_gauss = 0.0
        self.imu_magnetometer_y_gauss = 0.0
        self.imu_magnetometer_z_gauss = 0.0
        self.imu_temperature_degC = 0.0

        # GPS data
        self.gps_timestamp_ns = 0
        self.gps_latitude_deg = 0.0
        self.gps_longitude_deg = 0.0
        self.gps_altitude_m = 0.0
        self.gps_horizontal_uncertainty_m = 0.0
        self.gps_vertical_uncertainty_m = 0.0
        self.gps_speed_m_s = 0.0
        self.gps_course_deg = 0.0
        self.gps_fix_type = 0
        self.gps_num_satellites = 0

        # Odometry data
        self.odom_timestamp_ns = 0
        self.odom_position_x_m = 0.0
        self.odom_position_y_m = 0.0
        self.odom_position_z_m = 0.0
        self.odom_velocity_x_m_s = 0.0
        self.odom_velocity_y_m_s = 0.0
        self.odom_velocity_z_m_s = 0.0
        self.odom_angular_velocity_x_rad_s = 0.0
        self.odom_angular_velocity_y_rad_s = 0.0
        self.odom_angular_velocity_z_rad_s = 0.0

        # 4x4 transformation matrix from body to raw camera (row-major)
        self.T_body_to_raw_camera = [
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
        ]

    def info(self):
        """Return message info with type 9 for NavigationData."""
        return MessageInfo(9)

    def msg_size(self):
        """Calculate total message size in bytes."""
        # MessageInfo + 1 uint64 (timestamp_ns)
        # + IMU: 1 uint64 + 10 floats
        # + GPS: 1 uint64 + 7 floats + 2 int32
        # + Odometry: 1 uint64 + 9 floats
        # + Transform: 16 floats
        size = self.info().msg_size()
        size += struct.calcsize("Q")  # timestamp_ns
        size += struct.calcsize("Q") + struct.calcsize("ffffffffff")  # IMU
        size += struct.calcsize("Q") + struct.calcsize("fffffff") + struct.calcsize("ii")  # GPS
        size += struct.calcsize("Q") + struct.calcsize("fffffffff")  # Odometry
        size += struct.calcsize("ffffffffffffffff")  # 4x4 matrix
        return size

    def read(self, buffer, original_offset=0):
        """
        Read NavigationData from buffer.

        Args:
            buffer: byte buffer to read from
            original_offset: starting offset in buffer

        Returns:
            offset after reading
        """
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, original_offset)
        if msg_info.is_different(self.info(), "NavigationData"):
            return original_offset

        # Read main timestamp
        self.timestamp_ns = struct.unpack_from("Q", buffer, offset)[0]
        offset += struct.calcsize("Q")

        # Read IMU data
        self.imu_timestamp_ns = struct.unpack_from("Q", buffer, offset)[0]
        offset += struct.calcsize("Q")
        imu_data = struct.unpack_from("ffffffffff", buffer, offset)
        self.imu_acceleration_x_m_s2, self.imu_acceleration_y_m_s2, self.imu_acceleration_z_m_s2 = imu_data[0:3]
        self.imu_gyro_x_rad_s, self.imu_gyro_y_rad_s, self.imu_gyro_z_rad_s = imu_data[3:6]
        self.imu_magnetometer_x_gauss, self.imu_magnetometer_y_gauss, self.imu_magnetometer_z_gauss = imu_data[6:9]
        self.imu_temperature_degC = imu_data[9]
        offset += struct.calcsize("ffffffffff")

        # Read GPS data
        self.gps_timestamp_ns = struct.unpack_from("Q", buffer, offset)[0]
        offset += struct.calcsize("Q")
        gps_floats = struct.unpack_from("fffffff", buffer, offset)
        self.gps_latitude_deg, self.gps_longitude_deg, self.gps_altitude_m = gps_floats[0:3]
        self.gps_horizontal_uncertainty_m, self.gps_vertical_uncertainty_m = gps_floats[3:5]
        self.gps_speed_m_s, self.gps_course_deg = gps_floats[5:7]
        offset += struct.calcsize("fffffff")
        gps_ints = struct.unpack_from("ii", buffer, offset)
        self.gps_fix_type, self.gps_num_satellites = gps_ints
        offset += struct.calcsize("ii")

        # Read Odometry data
        self.odom_timestamp_ns = struct.unpack_from("Q", buffer, offset)[0]
        offset += struct.calcsize("Q")
        odom_data = struct.unpack_from("fffffffff", buffer, offset)
        self.odom_position_x_m, self.odom_position_y_m, self.odom_position_z_m = odom_data[0:3]
        self.odom_velocity_x_m_s, self.odom_velocity_y_m_s, self.odom_velocity_z_m_s = odom_data[3:6]
        self.odom_angular_velocity_x_rad_s, self.odom_angular_velocity_y_rad_s, self.odom_angular_velocity_z_rad_s = odom_data[
            6:9]
        offset += struct.calcsize("fffffffff")

        # Read transformation matrix
        self.T_body_to_raw_camera = list(struct.unpack_from("ffffffffffffffff", buffer, offset))
        offset += struct.calcsize("ffffffffffffffff")

        return offset

    def write(self, buffer, original_offset=0):
        """
        Write NavigationData to buffer.

        Args:
            buffer: byte buffer to write to
            original_offset: starting offset in buffer

        Returns:
            offset after writing
        """
        offset = self.info().write(buffer, original_offset)

        # Write main timestamp
        struct.pack_into("Q", buffer, offset, self.timestamp_ns)
        offset += struct.calcsize("Q")

        # Write IMU data
        struct.pack_into("Q", buffer, offset, self.imu_timestamp_ns)
        offset += struct.calcsize("Q")
        struct.pack_into("ffffffffff", buffer, offset,
                         self.imu_acceleration_x_m_s2, self.imu_acceleration_y_m_s2, self.imu_acceleration_z_m_s2,
                         self.imu_gyro_x_rad_s, self.imu_gyro_y_rad_s, self.imu_gyro_z_rad_s,
                         self.imu_magnetometer_x_gauss, self.imu_magnetometer_y_gauss, self.imu_magnetometer_z_gauss,
                         self.imu_temperature_degC)
        offset += struct.calcsize("ffffffffff")

        # Write GPS data
        struct.pack_into("Q", buffer, offset, self.gps_timestamp_ns)
        offset += struct.calcsize("Q")
        struct.pack_into("fffffff", buffer, offset,
                         self.gps_latitude_deg, self.gps_longitude_deg, self.gps_altitude_m,
                         self.gps_horizontal_uncertainty_m, self.gps_vertical_uncertainty_m,
                         self.gps_speed_m_s, self.gps_course_deg)
        offset += struct.calcsize("fffffff")
        struct.pack_into("ii", buffer, offset, self.gps_fix_type, self.gps_num_satellites)
        offset += struct.calcsize("ii")

        # Write Odometry data
        struct.pack_into("Q", buffer, offset, self.odom_timestamp_ns)
        offset += struct.calcsize("Q")
        struct.pack_into("fffffffff", buffer, offset,
                         self.odom_position_x_m, self.odom_position_y_m, self.odom_position_z_m,
                         self.odom_velocity_x_m_s, self.odom_velocity_y_m_s, self.odom_velocity_z_m_s,
                         self.odom_angular_velocity_x_rad_s, self.odom_angular_velocity_y_rad_s,
                         self.odom_angular_velocity_z_rad_s)
        offset += struct.calcsize("fffffffff")

        # Write transformation matrix
        struct.pack_into("ffffffffffffffff", buffer, offset, *self.T_body_to_raw_camera)
        offset += struct.calcsize("ffffffffffffffff")

        return offset

    def __str__(self):
        return (
            f"NavigationData(timestamp_ns={self.timestamp_ns}, "
            f"imu_accel=[{self.imu_acceleration_x_m_s2:.2f}, {self.imu_acceleration_y_m_s2:.2f}, {self.imu_acceleration_z_m_s2:.2f}] m/s², "
            f"gps_pos=[{self.gps_latitude_deg:.6f}°, {self.gps_longitude_deg:.6f}°, {self.gps_altitude_m:.2f}m], "
            f"odom_pos=[{self.odom_position_x_m:.2f}, {self.odom_position_y_m:.2f}, {self.odom_position_z_m:.2f}] m)"
        )

    def __repr__(self):
        return self.__str__()
