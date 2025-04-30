import struct

import numpy as np

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


class PointCloud:
    HEADER_SIZE = 512

    def __init__(self, time=0, frame_id=0, points=None):
        self.time = time
        self.frame_id = frame_id
        self.points = points

    def info(self):
        return MessageInfo(4)

    def msg_size(self):
        return PointCloud.HEADER_SIZE + self.points.nbytes

    def read(self, buffer, original_offset=0):
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, original_offset)
        if msg_info.is_different(self.info(), "PointCloud"):
            return original_offset

        self.time, self.frame_id, num_points = struct.unpack_from('QQQ', buffer, offset)
        if num_points > 1e8:
            print(f"According to the message, the point cloud has {num_points}.  "
                  "We are ignoring this message so that you don't run out of memory.")
            return original_offset

        self.points = np.frombuffer(buffer, dtype=np.float32, count=num_points * 3,
                                    offset=original_offset + PointCloud.HEADER_SIZE).reshape(num_points, 3)
        return original_offset + self.msg_size()

    def write(self, buffer, original_offset):
        time, frame_id, points = self.time, self.frame_id, self.points
        if points.ndim != 2 or points.shape[1] != 3:
            print(f"Cannot write point_cloud, it should be a numpy array of size N x 3")
            return original_offset
        offset = self.info().write(buffer, original_offset)
        struct.pack_into('QQQ', buffer, offset, time, frame_id, len(points))
        offset = original_offset + PointCloud.HEADER_SIZE
        buffer[offset:offset + points.nbytes] = points.tobytes()
        return original_offset + self.msg_size()
