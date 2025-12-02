import struct
from enum import IntEnum

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


class Severity(IntEnum):
    INFO = 0
    WARNING = 1
    ERROR = 2


def _read_c_string(b: bytes) -> str:
    head = b.split(b"\x00", 1)[0]
    return head.decode("utf-8", errors="replace")


class Finding:
    NUM_BYTES = 320

    def __init__(self, domain="", key="", severity=Severity.INFO, message="", value=0.0, unit=""):
        self.domain = domain[:31]
        self.key = key[:127]
        self.message = message[:127]
        self.unit = unit[:15]
        self.severity = Severity(severity) if isinstance(severity, int) else severity
        self.value = float(value)

    def to_bytes(self) -> bytes:
        # Fixed-size C strings, zero-padded
        domain_bytes = self.domain.encode("utf-8")[:31].ljust(32, b"\x00")
        key_bytes = self.key.encode("utf-8")[:127].ljust(128, b"\x00")
        message_bytes = self.message.encode("utf-8")[:127].ljust(128, b"\x00")
        unit_bytes = self.unit.encode("utf-8")[:15].ljust(16, b"\x00")

        payload = (
                domain_bytes
                + key_bytes
                + message_bytes
                + unit_bytes
                + struct.pack("<d", self.value)  # offset 304
                + struct.pack("<B", int(self.severity))  # offset 312
                + b"\x00" * 7  # pad to 320
        )
        assert len(payload) == Finding.NUM_BYTES
        return payload

    @staticmethod
    def from_bytes(buffer: bytes, offset: int = 0):
        base = offset
        domain_b = buffer[base:base + 32]
        key_b = buffer[base + 32:base + 160]
        message_b = buffer[base + 160:base + 288]
        unit_b = buffer[base + 288:base + 304]

        domain = _read_c_string(domain_b)
        key = _read_c_string(key_b)
        message = _read_c_string(message_b)
        unit = _read_c_string(unit_b)

        (value,) = struct.unpack_from("<d", buffer, base + 304)
        (sev_raw,) = struct.unpack_from("<B", buffer, base + 312)
        try:
            sev = Severity(sev_raw)
        except ValueError:
            sev = Severity.INFO  # graceful fallback

        finding = Finding(domain, key, sev, message, value, unit)
        return finding, base + Finding.NUM_BYTES

    def __repr__(self):
        return (
            f"Finding(domain='{self.domain}', key='{self.key}', "
            f"severity={self.severity.name}, message='{self.message}', "
            f"value={self.value}, unit='{self.unit}')"
        )


class QAFindings:
    HEADER_SIZE = 64

    def __init__(self, time: int = 0, frame_id: int = 0, findings=None):
        self.time = time
        self.frame_id = frame_id
        self.findings = list(findings) if findings else []

    def info(self):
        return MessageInfo(9)

    def findings_bytes(self) -> int:
        return len(self.findings) * Finding.NUM_BYTES

    def msg_size(self) -> int:
        return QAFindings.HEADER_SIZE + self.findings_bytes()

    def empty(self) -> bool:
        return len(self.findings) == 0

    def read(self, buffer: bytes, original_offset: int = 0) -> int:
        # Parse MessageInfo at start of header
        msg_info = MessageInfo()
        off_after_info = msg_info.read(buffer, original_offset)
        if msg_info.is_different(self.info(), "QAFindings"):
            # Wrong message type/version; leave object unchanged
            return original_offset

        # Three uint64s right after MessageInfo, little-endian
        self.time, self.frame_id, num_findings = struct.unpack_from("<QQQ", buffer, off_after_info)

        # Findings payload begins exactly at HEADER_SIZE
        offset = original_offset + QAFindings.HEADER_SIZE
        self.findings = []
        for _ in range(num_findings):
            finding, offset = Finding.from_bytes(buffer, offset)
            self.findings.append(finding)

        return original_offset + self.msg_size()

    def write(self, buffer: bytearray, original_offset: int = 0) -> int:
        # Write MessageInfo at start of header
        offset = self.info().write(buffer, original_offset)

        # Write time, frame_id, num_findings directly after MessageInfo (still within the 64-byte header)
        struct.pack_into("<QQQ", buffer, offset, self.time, self.frame_id, len(self.findings))

        # Findings payload begins at HEADER_SIZE
        offset = original_offset + QAFindings.HEADER_SIZE
        for f in self.findings:
            b = f.to_bytes()
            buffer[offset:offset + Finding.NUM_BYTES] = b
            offset += Finding.NUM_BYTES

        return original_offset + self.msg_size()

    def __repr__(self):
        return f"QAFindings(time={self.time}, frame_id={self.frame_id}, findings={self.findings})"
