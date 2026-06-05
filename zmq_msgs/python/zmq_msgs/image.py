import numpy as np
import struct

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
    # Fields: time, frame_id, rows, cols, type, step, cvt_to_bgr_code, additional_field_size.
    HEADER_STRUCT_FORMAT = "=QQIIIIBH"

    def __init__(self, time=0, frame_id=0, cvt_to_bgr_code=COLOR_CONVERSION.UNSPECIFIED, img=None, step=0):
        self.time = time
        self.frame_id = frame_id
        self.cvt_to_bgr_code = cvt_to_bgr_code
        self.img: np.ndarray = img
        self.step = step
        self._additional_field: bytearray = bytearray()

    def info(self) -> MessageInfo:
        return MessageInfo(0)

    def msg_size(self):
        return StampedImage.HEADER_SIZE + self.img.nbytes + len(self._additional_field)

    @property
    def additional_field(self) -> bytearray:
        """Returns the additional field as a bytearray."""
        return self._additional_field

    @additional_field.setter
    def additional_field(self, value: bytearray | bytes):
        """Sets the additional field to a bytearray."""
        if isinstance(value, bytes):
            value = bytearray(value)
        if not isinstance(value, bytearray):
            raise TypeError("Additional field must be a bytearray or bytes.")
        self._additional_field = value

    def read(self, buffer: bytes, original_offset: int = 0):
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, original_offset)
        if msg_info.is_different(self.info(), "StampedImage"):
            return None

        unpacked = struct.unpack_from(self.HEADER_STRUCT_FORMAT, buffer, offset)
        self.time = unpacked[0]
        self.frame_id = unpacked[1]
        rows = unpacked[2]
        cols = unpacked[3]
        cv_type = unpacked[4]
        step = unpacked[5]
        self.cvt_to_bgr_code = unpacked[6]
        n_additional = unpacked[7]

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
        row_bytes = cols * channels * np.dtype(dtype).itemsize

        if step < row_bytes:
            print(
                f"According to the message, the row step of {step} bytes is smaller than a tightly packed "
                f"row of {row_bytes} bytes, which is invalid. We are ignoring this message."
            )
            return None

        # Guard against an implausibly large payload (500 MB), e.g. a corrupt step.
        image_nbytes = step * rows
        if image_nbytes > 500 * 1024 * 1024:
            print(
                f"According to the message, the image payload is {image_nbytes} bytes, which is implausibly "
                "large. We are ignoring this message so that you don't run out of memory."
            )
            return None

        if step == row_bytes:
            # Tightly packed rows.
            self.img = np.frombuffer(
                buffer,
                dtype=dtype,
                count=rows * cols * channels,
                offset=offset,
            ).reshape(rows, cols, channels)
        else:
            # Padded rows (e.g. a Tegra GpuMat): copy the valid leading bytes out of each padded row.
            # The copy is what drops the padding -- the slice on its own is just a strided view.
            raw = np.frombuffer(buffer, dtype=np.uint8, count=step * rows, offset=offset).reshape(rows, step)
            self.img = raw[:, :row_bytes].copy().view(dtype).reshape(rows, cols, channels)
        # We always store a tightly packed copy in memory, so its stride is the packed row size.
        self.step = row_bytes
        offset += image_nbytes

        # Read the additional field
        self._additional_field = bytearray(buffer[offset:offset + n_additional])
        offset += n_additional

        return offset

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
            self.HEADER_STRUCT_FORMAT,
            buffer,
            offset,
            time,
            frame_id,
            rows,
            cols,
            encode_cv_type(channels, img.dtype),
            cols * channels * img.dtype.itemsize,  # step: numpy rows are tightly packed, so this is the stride.
            cvt_to_bgr_code,
            len(self._additional_field),
        )
        offset = original_offset + StampedImage.HEADER_SIZE

        # Write image data
        buffer[offset: offset + img.nbytes] = img.tobytes()
        offset += img.nbytes

        # Write additional field
        buffer[offset: offset + len(self._additional_field)] = self._additional_field
        offset += len(self._additional_field)

        assert offset == original_offset + self.msg_size()
        return original_offset + self.msg_size()
