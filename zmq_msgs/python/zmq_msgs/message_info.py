import struct


class MessageInfo:
    MAJOR_VERSION = 0
    MINOR_VERSION = 1

    def __init__(self, message_type=-1):  # -1 denotes INVALID
        self.message_type = message_type
        self.major_version = self.MAJOR_VERSION
        self.minor_version = self.MINOR_VERSION

    def __eq__(self, other):
        return (
            self.message_type == other.message_type
            and self.major_version == other.major_version
            and self.minor_version == other.minor_version
        )

    def __ne__(self, other):
        return not self == other

    def is_different(self, other, expected_type):
        if self.message_type != other.message_type:
            print(f"This message is not a {expected_type} message.")
            return True
        if self.major_version != other.major_version:
            print(
                f"{expected_type} message major versions differ: {self.major_version} != {other.major_version}"
            )
            return True
        if self.minor_version != other.minor_version:
            print(
                f"{expected_type} message minor versions differ: {self.minor_version} != {other.minor_version}"
            )
            return True
        return False

    def msg_size(self):
        return struct.calcsize("HBB")

    def read(self, buffer, offset=0):
        (
            self.message_type,
            self.major_version,
            self.minor_version,
        ) = struct.unpack_from("HBB", buffer, offset)
        return offset + self.msg_size()

    def write(self, buffer, offset):
        buffer[offset : offset + self.msg_size()] = struct.pack(
            "HBB", self.message_type, self.major_version, self.minor_version
        )
        return offset + self.msg_size()
