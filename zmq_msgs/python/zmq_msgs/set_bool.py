import struct

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


class SetBoolRequest:
    def __init__(self, val=False):
        self.val = val

    def info(self):
        return MessageInfo(6)

    def msg_size(self):
        return self.info().msg_size() + struct.calcsize("?")

    def read(self, buffer, offset=0):
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, offset)
        if msg_info.is_different(self.info(), "SetBoolRequest"):
            return None
        (self.val,) = struct.unpack_from("?", buffer, offset)
        return offset + struct.calcsize("?")

    def write(self, buffer, offset):
        offset = self.info().write(buffer, offset)
        new_offset = offset + struct.calcsize("?")
        buffer[offset:new_offset] = struct.pack("?", self.val)
        return new_offset


class SetBoolResponse:
    def __init__(self, success=False):
        self.success = success

    def info(self):
        return MessageInfo(7)

    def msg_size(self):
        return self.info().msg_size() + struct.calcsize("?")

    def read(self, buffer, offset=0):
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, offset)
        if msg_info.is_different(self.info(), "SetBoolResponse"):
            return None
        (self.success,) = struct.unpack_from("?", buffer, offset)
        return offset + struct.calcsize("?")

    def write(self, buffer, offset):
        offset = self.info().write(buffer, offset)
        new_offset = offset + struct.calcsize("?")
        buffer[offset:new_offset] = struct.pack("?", self.success)
        return new_offset
