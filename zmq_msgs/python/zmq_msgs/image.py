import struct

import numpy as np

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


def decode_cv_type(cv_type):
    if cv_type == 1:
        return 1, np.int8
    if cv_type == 17:
        return 3, np.int8
    if cv_type == 0:
        return 1, np.uint8
    if cv_type == 16:
        return 3, np.uint8
    if cv_type == 3:
        return 1, np.int16
    if cv_type == 19:
        return 3, np.int16
    if cv_type == 2:
        return 1, np.uint16
    if cv_type == 18:
        return 3, np.uint16
    if cv_type == 5:
        return 1, np.float32
    if cv_type == 21:
        return 3, np.float32
    if cv_type == 6:
        return 1, np.float64
    if cv_type == 22:
        return 3, np.float64
    assert False, "unknown cv type"


def encode_cv_type(channels, dtype):
    if dtype == np.int8:
        if channels == 1:
            return 1
        elif channels == 3:
            return 17
    elif dtype == np.uint8:
        if channels == 1:
            return 0
        elif channels == 3:
            return 16
    elif dtype == np.int16:
        if channels == 1:
            return 3
        elif channels == 3:
            return 19
    elif dtype == np.uint16:
        if channels == 1:
            return 2
        elif channels == 3:
            return 18
    elif dtype == np.float32:
        if channels == 1:
            return 5
        elif channels == 3:
            return 21
    elif dtype == np.float64:
        if channels == 1:
            return 6
        elif channels == 3:
            return 22
    assert False, "unknown combination of channels and dtype"


class StampedImage:
    HEADER_SIZE = 64

    def __init__(self, ):
        self.time = 0
        self.frame_id = 0
        self.img = None

    @staticmethod
    def msg_size(rows, cols, channels, dtype):
        return StampedImage.HEADER_SIZE + StampedImage.data_size(rows, cols, channels, dtype)

    @staticmethod
    def infos():
        return MessageInfo(0)

    def info(self, ):
        return StampedImage.infos()

    @staticmethod
    def data_size(rows, cols, channels, dtype):
        return rows * cols * channels * np.dtype(dtype).itemsize

    def is_empty(self):
        return self.img.size == 0

    def read(self, buffer, original_offset=0):
        info, offset = MessageInfo.reads(buffer, original_offset)
        if info.is_different(self.info(), "StampedImage"):
            return None

        self.time, self.frame_id, rows, cols, cv_type = struct.unpack_from('QQIII', buffer, offset)
        if rows * cols > 1e8:
            print("According to the message, the image has the impossibly large of dimensions "
                  f"{rows} x {cols}. We are ignoring this message so that you don't run out of memory.")
            return

        # Convert opencv type to something more understandable
        channels, dtype = decode_cv_type(cv_type)
        self.img = np.frombuffer(buffer, dtype=dtype, count=rows * cols * channels,
                                 offset=original_offset + StampedImage.HEADER_SIZE).reshape(rows, cols, channels)
        return original_offset + StampedImage.msg_size(rows, cols, channels, dtype)

    @staticmethod
    def reads(buffer, offset=0):
        image = StampedImage()
        offset = image.read(buffer, offset)
        return image, offset

    @staticmethod
    def writes(buffer, original_offset, time, frame_id, img):
        if img.ndim == 2:
            rows, cols = img.shape
            channels = 1
        elif img.ndim == 3:
            rows, cols, channels = img.shape
        else:
            print(f"Cannot write this image, it has either less than 2 or more than 3 dimensions")
            return
        offset = StampedImage.infos().write(buffer, original_offset)
        struct.pack_into('QQIII', buffer, offset, time, frame_id, rows, cols, encode_cv_type(channels, img.dtype))
        offset = original_offset + StampedImage.HEADER_SIZE
        buffer[offset:offset + img.nbytes] = img
        return offset + img.nbytes

    def write(self, buffer, offset):
        return self.writes(buffer, offset, self.time, self.frame_id, self.img)
