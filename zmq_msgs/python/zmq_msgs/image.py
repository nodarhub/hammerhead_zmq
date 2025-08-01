import struct

import numpy as np

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


# Note that one could import these types from the cv2 package.
# We intentionally don't do that here because you would be importing
# an extra package just for these type definitions
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
    class COLOR_CONVERSION:
        BGR2BGR = 253
        INCONVERTIBLE = 254
        UNSPECIFIED = 255

    HEADER_SIZE = 64

    def __init__(self, time=0, frame_id=0, cvt_to_bgr_code=COLOR_CONVERSION.UNSPECIFIED, img=None):
        self.time = time
        self.frame_id = frame_id
        self.cvt_to_bgr_code = cvt_to_bgr_code
        self.img: np.ndarray = img
        self.additional_field: bytearray = bytearray()

    def info(self) -> MessageInfo:
        return MessageInfo(0)

    def msg_size(self):
        return StampedImage.HEADER_SIZE + self.img.nbytes + len(self.additional_field)

    def read(self, buffer: bytes, original_offset: int = 0):
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, original_offset)
        if msg_info.is_different(self.info(), "StampedImage"):
            return None

        unpacked = struct.unpack_from("QQIIIBH", buffer, offset)
        self.time = unpacked[0]
        self.frame_id = unpacked[1]
        rows = unpacked[2]
        cols = unpacked[3]
        cv_type = unpacked[4]
        self.cvt_to_bgr_code = unpacked[5]
        n_additional = unpacked[6]

        if rows * cols > 1e8:
            print(
                "According to the message, the image has the impossibly large of dimensions "
                f"{rows} x {cols}. "
                "We are ignoring this message so that you don't run out of memory."
            )
            return None
        offset = original_offset + StampedImage.HEADER_SIZE

        # Convert opencv type to something more understandable
        channels, dtype = decode_cv_type(cv_type)
        self.img = np.frombuffer(
            buffer,
            dtype=dtype,
            count=rows * cols * channels,
            offset=offset,
        ).reshape(rows, cols, channels)
        offset += self.img.nbytes

        # Read the additional field
        self.additional_field = bytearray(buffer[offset:offset + n_additional])
        offset += n_additional

        assert offset == original_offset + self.msg_size()
        return original_offset + self.msg_size()

    def write(self, buffer: bytearray, original_offset: int):
        time, frame_id, cvt_to_bgr_code, img = (
            self.time,
            self.frame_id,
            self.cvt_to_bgr_code,
            self.img,
        )
        if img.ndim == 2:
            rows, cols = img.shape
            channels = 1
        elif img.ndim == 3:
            rows, cols, channels = img.shape
        else:
            print("Cannot write this image, it has either <2 dimensions, or >3 dimensions")
            return None

        # Write header
        offset = self.info().write(buffer, original_offset)
        struct.pack_into(
            "QQIIIBH",
            buffer,
            offset,
            time,
            frame_id,
            rows,
            cols,
            encode_cv_type(channels, img.dtype),
            cvt_to_bgr_code,
            len(self.additional_field),
        )
        offset = original_offset + StampedImage.HEADER_SIZE

        # Write image data
        buffer[offset : offset + img.nbytes] = img.tobytes()
        offset += img.nbytes

        # Write additional field
        buffer[offset : offset + len(self.additional_field)] = self.additional_field
        offset += len(self.additional_field)

        assert offset == original_offset + self.msg_size()
        return original_offset + self.msg_size()
