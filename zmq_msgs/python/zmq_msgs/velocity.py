import struct

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


class VelocityData:
    """
    Message for velocity data with rotation matrix.

    Velocity is in odometry coordinate system [m/s].
    Rotation matrix converts from odometry coordinate system to Nodar raw camera coordinate system.
    """

    def __init__(self, time=0, velocity=None, rotation_to_nodar_raw=None):
        """
        Initialize VelocityData.

        Args:
            time: timestamp in nanoseconds
            velocity: list/tuple of 3 floats [vx, vy, vz] in m/s
            rotation_to_nodar_raw: list/tuple of 9 floats representing 3x3 rotation matrix in row-major order
        """
        self.time = time
        self.velocity = velocity if velocity is not None else [0.0, 0.0, 0.0]
        self.rotation_to_nodar_raw = rotation_to_nodar_raw if rotation_to_nodar_raw is not None else [
            1.0, 0.0, 0.0,  # Identity matrix
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0
        ]

        # Ensure proper dimensions
        assert len(self.velocity) == 3, "Velocity must have exactly 3 components"
        assert len(self.rotation_to_nodar_raw) == 9, "Rotation matrix must have exactly 9 components"

    def info(self):
        """Return message info with type 9 for VelocityData."""
        return MessageInfo(9)

    def msg_size(self):
        """Calculate total message size in bytes."""
        # MessageInfo + timestamp (uint64) + 3 floats (velocity) + 9 floats (rotation)
        return self.info().msg_size() + struct.calcsize("Q") + struct.calcsize("fff") + struct.calcsize("fffffffff")

    def read(self, buffer, original_offset=0):
        """
        Read VelocityData from buffer.

        Args:
            buffer: byte buffer to read from
            original_offset: starting offset in buffer

        Returns:
            offset after reading
        """
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, original_offset)
        if msg_info.is_different(self.info(), "VelocityData"):
            return original_offset

        # Read timestamp
        self.time = struct.unpack_from("Q", buffer, offset)[0]
        offset += struct.calcsize("Q")

        # Read velocity array (3 floats)
        self.velocity = list(struct.unpack_from("fff", buffer, offset))
        offset += struct.calcsize("fff")

        # Read rotation matrix (9 floats)
        self.rotation_to_nodar_raw = list(struct.unpack_from("fffffffff", buffer, offset))
        offset += struct.calcsize("fffffffff")

        return offset

    def write(self, buffer, original_offset=0):
        """
        Write VelocityData to buffer.

        Args:
            buffer: byte buffer to write to
            original_offset: starting offset in buffer

        Returns:
            offset after writing
        """
        offset = self.info().write(buffer, original_offset)

        # Write timestamp
        struct.pack_into("Q", buffer, offset, self.time)
        offset += struct.calcsize("Q")

        # Write velocity array (3 floats)
        struct.pack_into("fff", buffer, offset, *self.velocity)
        offset += struct.calcsize("fff")

        # Write rotation matrix (9 floats)
        struct.pack_into("fffffffff", buffer, offset, *self.rotation_to_nodar_raw)
        offset += struct.calcsize("fffffffff")

        return offset

    def __str__(self):
        return (
            f"VelocityData(time={self.time}, "
            f"velocity=[{self.velocity[0]:.2f}, {self.velocity[1]:.2f}, {self.velocity[2]:.2f}], "
            f"rotation_to_nodar_raw={self.rotation_to_nodar_raw})"
        )

    def __repr__(self):
        return self.__str__()