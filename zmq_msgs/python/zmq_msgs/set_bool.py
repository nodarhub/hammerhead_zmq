import struct

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


class SetBoolRequest:

    def __init__(self, val=False):
        self.val = val

    def read(self, buffer, offset=0):
        info, offset = MessageInfo.reads(buffer, offset)
        if info.is_different(self.info(), "SetBoolRequest"):
            return None
        self.val, = struct.unpack_from('?', buffer, offset)
        return offset + struct.calcsize('?')

    @staticmethod
    def msg_size():
        return MessageInfo.msg_size() + struct.calcsize('?')

    @staticmethod
    def infos():
        return MessageInfo(6)

    def info(self, ):
        return SetBoolRequest.infos()

    @staticmethod
    def reads(buffer, offset=0):
        request = SetBoolRequest()
        new_offset = request.read(buffer, offset)
        return request, new_offset

    @staticmethod
    def writes(buffer, offset, val):
        offset = SetBoolRequest.infos().write(buffer, offset)
        buffer[offset:offset + struct.calcsize('?')] = struct.pack('?', val)
        return offset + struct.calcsize('?')

    def write(self, buffer, offset=0):
        return self.writes(buffer, offset, self.val)


class SetBoolResponse:

    def __init__(self, success=False):
        self.success = success

    def read(self, buffer, offset=0):
        info, offset = MessageInfo.reads(buffer, offset)
        if info.is_different(self.info(), "SetBoolResponse"):
            return None
        self.success, = struct.unpack_from('?', buffer, offset)
        return offset + struct.calcsize('?')

    @staticmethod
    def msg_size():
        return MessageInfo.msg_size() + struct.calcsize('?')

    @staticmethod
    def infos():
        return MessageInfo(7)

    def info(self, ):
        return SetBoolResponse.infos()

    @staticmethod
    def reads(buffer, offset=0):
        response = SetBoolResponse()
        new_offset = response.read(buffer, offset)
        return response, new_offset

    @staticmethod
    def writes(buffer, offset, success):
        offset = SetBoolResponse.infos().write(buffer, offset)
        buffer[offset:offset + struct.calcsize('?')] = struct.pack('?', success)
        return offset + struct.calcsize('?')

    def write(self, buffer, offset=0):
        return self.writes(buffer, offset, self.success)
