import struct

import numpy as np

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


class PointCloudRGB:
    HEADER_SIZE = 512

    def __init__(self, time=0, frame_id=0, points=None, colors=None):
        self.time = time
        self.frame_id = frame_id
        self.colors = colors

    def info(self):
        return MessageInfo(5)

    def msg_size(self):
        return PointCloudRGB.HEADER_SIZE + self.points.nbytes + self.colors.nbytes

    def read(self, buffer, original_offset=0):
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, original_offset)
        if msg_info.is_different(self.info(), "PointCloudRGB"):
            return None

        self.time, self.frame_id, num_points = struct.unpack_from("QQQ", buffer, offset)
        if num_points > 1e8:
            print(
                f"According to the message, the point cloud has {num_points}.  "
                "We are ignoring this message so that you don't run out of memory."
            )
            return

        self.points = np.frombuffer(
            buffer,
            dtype=np.float32,
            count=num_points * 3,
            offset=original_offset + PointCloudRGB.HEADER_SIZE,
        ).reshape(num_points, 3)
        self.colors = np.frombuffer(
            buffer,
            dtype=np.float32,
            count=num_points * 3,
            offset=original_offset + PointCloudRGB.HEADER_SIZE + self.points.nbytes,
        )
        self.colors = self.colors.reshape(num_points, 3)
        return original_offset + self.msg_size()

    def write(self, buffer, original_offset):
        time, frame_id, points, colors = self.time, self.frame_id, self.points, self.colors
        if points.ndim != 2 or points.shape[1] != 3:
            print(f"Cannot write point_cloud, it should be a numpy array of size N x 3")
            return
        if colors.ndim != 2 or colors.shape[1] != 3:
            print(
                f"Cannot write point_cloud, the colors array should be a numpy array of size N x 3"
            )
            return
        if len(points) != len(colors):
            print(
                f"Cannot write point_cloud, the points and colors arrays should be the same length"
            )
        offset = self.info().write(buffer, original_offset)
        struct.pack_into("QQIII", buffer, offset, time, frame_id, len(points))
        offset = original_offset + PointCloudRGB.HEADER_SIZE
        buffer[offset : offset + points.nbytes] = points.tobytes()
        offset += points.nbytes
        buffer[offset : offset + colors.nbytes] = colors.tobytes()
        return original_offset + self.msg_size()
