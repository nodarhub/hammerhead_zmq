import struct

import numpy as np

try:
    from zmq_msgs.image import StampedImage
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .image import StampedImage
    from .message_info import MessageInfo


class PointCloudSoup:
    def __init__(self,
                 time=0,
                 frame_id=0,
                 baseline=1,
                 focal_length=1000,
                 disparity_to_depth4x4=None,
                 rotation_disparity_to_raw_cam=None,
                 rotation_world_to_raw_cam=None,
                 rectified=None,
                 disparity=None, ):
        self.time = time  # uint64_t
        self.frame_id = frame_id  # uint64_t
        self.baseline = baseline  # double
        self.focal_length = focal_length  # double
        self.disparity_to_depth4x4 = disparity_to_depth4x4  # float array with 16 entries
        self.rotation_disparity_to_raw_cam = rotation_disparity_to_raw_cam  # float array with 9 entries
        self.rotation_world_to_raw_cam = rotation_world_to_raw_cam  # float array with 9 entries
        self.rectified = rectified
        self.disparity = disparity

    def info(self):
        return MessageInfo(1)

    def msg_size(self):
        return (self.info().msg_size()
                + struct.calcsize('QQdd')  # time, frame_id, baseline, focal_length
                + 16 * np.dtype(np.float32).itemsize  # disparity_to_depth4x4
                + 9 * np.dtype(np.float32).itemsize  # rotation_disparity_to_raw_cam
                + 9 * np.dtype(np.float32).itemsize  # rotation_world_to_raw_cam
                + self.rectified.msg_size()
                + self.disparity.msg_size())

    def read(self, buffer, offset=0):
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, offset)
        if msg_info.is_different(self.info(), "PointCloudSoup"):
            return None

        self.time, self.frame_id, self.baseline, self.focal_length = struct.unpack_from('QQdd', buffer, offset)
        offset += struct.calcsize('QQdd')
        self.disparity_to_depth4x4 = np.frombuffer(
            buffer, dtype=np.float32, count=16, offset=offset
        ).reshape(4, 4)
        offset += self.disparity_to_depth4x4.nbytes
        self.rotation_disparity_to_raw_cam = np.frombuffer(
            buffer, dtype=np.float32, count=9, offset=offset
        ).reshape(3, 3)
        offset += self.rotation_disparity_to_raw_cam.nbytes
        self.rotation_world_to_raw_cam = np.frombuffer(
            buffer, dtype=np.float32, count=9, offset=offset
        ).reshape(3, 3)
        offset += self.rotation_world_to_raw_cam.nbytes
        self.rectified = StampedImage()
        offset = self.rectified.read(buffer, offset)
        self.disparity = StampedImage()
        offset = self.disparity.read(buffer, offset)
        return offset

    def write(self, buffer, offset):
        offset = self.info().write(buffer, offset)
        struct.pack_into('QQdd', buffer, offset, self.time, self.frame_id, self.baseline, self.focal_length)
        offset += struct.calcsize('QQdd')
        buffer[offset:offset + self.disparity_to_depth4x4.nbytes] = self.disparity_to_depth4x4.tobytes()
        offset += self.disparity_to_depth4x4.nbytes
        buffer[offset:offset + self.rotation_disparity_to_raw_cam.nbytes] = self.rotation_disparity_to_raw_cam.tobytes()
        offset += self.rotation_disparity_to_raw_cam.nbytes
        buffer[offset:offset + self.rotation_world_to_raw_cam.nbytes] = self.rotation_world_to_raw_cam.tobytes()
        offset += self.rotation_world_to_raw_cam.nbytes
        offset = self.rectified.write(buffer, offset)
        offset = self.disparity.write(buffer, offset)
        return offset
