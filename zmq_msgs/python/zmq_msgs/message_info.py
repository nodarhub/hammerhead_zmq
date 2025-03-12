import struct


class MessageInfo:
    MAJOR_VERSION = 0
    MINOR_VERSION = 1

    def __init__(self, message_type):
        self.message_type = message_type
        self.major_version = self.MAJOR_VERSION
        self.minor_version = self.MINOR_VERSION

    def __eq__(self, other):
        return (self.message_type == other.message_type and
                self.major_version == other.major_version and
                self.minor_version == other.minor_version)

    def __ne__(self, other):
        return not self == other

    def is_different(self, other, expected_type):
        if self.message_type != other.message_type:
            print(f"This message is not a {expected_type} message.")
            return True
        if self.major_version != other.major_version:
            print(f"{expected_type} message major versions differ: {self.major_version} != {other.major_version}"
                  )
            return True
        if self.minor_version != other.minor_version:
            print(f"{expected_type} message minor versions differ: {self.minor_version} != {other.minor_version}")
            return True
        return False

    @staticmethod
    def msg_size():
        return struct.calcsize('HBB')

    def read(self, buffer, offset=0):
        self.message_type, self.major_version, self.minor_version,  = struct.unpack_from('HBB', buffer, offset)
        return offset + self.msg_size()

    @staticmethod
    def reads(buffer, offset=0):
        message_info = MessageInfo(0)
        new_offset = message_info.read(buffer, offset)
        return message_info, new_offset

    @staticmethod
    def writes(buffer, offset, message_type, major_version, minor_version):
        buffer[offset:offset + MessageInfo.msg_size()] = struct.pack('HBB', message_type, major_version, minor_version)
        return offset + MessageInfo.msg_size()

    def write(self, buffer, offset=0):
        return self.writes(buffer, offset, self.message_type, self.major_version, self.minor_version)
