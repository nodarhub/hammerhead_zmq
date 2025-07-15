import struct

try:
    from zmq_msgs.message_info import MessageInfo
except ImportError:
    from .message_info import MessageInfo


class Vec2:
    NUM_BYTES = struct.calcsize("ff")

    def __init__(self, x=0.0, z=0.0):
        self.x = x
        self.z = z

    def to_bytes(self):
        return struct.pack("ff", self.x, self.z)

    @staticmethod
    def from_bytes(buffer, offset=0):
        x, z = struct.unpack_from("ff", buffer, offset)
        return Vec2(x, z), offset + Vec2.NUM_BYTES

    def __str__(self):
        return f"({self.x:.2f}, {self.z:.2f})"

    def __repr__(self):
        return self.__str__()


class BoundingBox:
    NUM_BYTES = 4 * Vec2.NUM_BYTES

    def __init__(self, points=None):
        if points is None:
            points = [Vec2() for _ in range(4)]
        assert len(points) == 4, "BoundingBox must have exactly 4 points."
        self.points = points

    def to_bytes(self):
        return b"".join(point.to_bytes() for point in self.points)

    @staticmethod
    def from_bytes(buffer, offset=0):
        points = []
        for _ in range(4):
            point, offset = Vec2.from_bytes(buffer, offset)
            points.append(point)
        return BoundingBox(points), offset

    def __str__(self):
        return f"{self.points}"

    def __repr__(self):
        return self.__str__()


class Obstacle:
    NUM_BYTES = BoundingBox.NUM_BYTES + Vec2.NUM_BYTES

    def __init__(self, bounding_box=None, velocity=None):
        if bounding_box is None:
            bounding_box = BoundingBox()
        if velocity is None:
            velocity = Vec2()
        self.bounding_box = bounding_box
        self.velocity = velocity

    def to_bytes(self):
        return self.bounding_box.to_bytes() + self.velocity.to_bytes()

    @staticmethod
    def from_bytes(buffer, offset=0):
        bounding_box, offset = BoundingBox.from_bytes(buffer, offset)
        velocity, offset = Vec2.from_bytes(buffer, offset)
        return Obstacle(bounding_box, velocity), offset

    def __str__(self):
        return f"Obstacle(bounding_box={self.bounding_box}, velocity={self.velocity})"

    def __repr__(self):
        return self.__str__()


class ObstacleData:
    HEADER_SIZE = 512

    def __init__(self, time=0, frame_id=0, obstacles=None):
        self.time = time
        self.frame_id = frame_id
        self.obstacles = obstacles if obstacles else []

    def info(self):
        return MessageInfo(8)

    def obstacle_bytes(self):
        return len(self.obstacles) * Obstacle.NUM_BYTES

    def msg_size(self):
        return ObstacleData.HEADER_SIZE + self.obstacle_bytes()

    def read(self, buffer, original_offset=0):
        msg_info = MessageInfo()
        offset = msg_info.read(buffer, original_offset)
        if msg_info.is_different(self.info(), "ObstacleData"):
            return original_offset

        self.time, self.frame_id, num_obstacles = struct.unpack_from("QQQ", buffer, offset)
        self.obstacles = []
        offset = original_offset + ObstacleData.HEADER_SIZE
        for _ in range(num_obstacles):
            obstacle, offset = Obstacle.from_bytes(buffer, offset)
            self.obstacles.append(obstacle)
        return original_offset + self.msg_size()

    def write(self, buffer, original_offset=0):
        offset = self.info().write(buffer, original_offset)
        struct.pack_into("QQQ", buffer, offset, self.time, self.frame_id, len(self.obstacles))
        offset = original_offset + ObstacleData.HEADER_SIZE
        for obstacle in self.obstacles:
            buffer[offset : offset + Obstacle.NUM_BYTES] = obstacle.to_bytes()
            offset += Obstacle.NUM_BYTES
        return original_offset + self.msg_size()

    def __str__(self):
        return (
            f"ObstacleData(time={self.time}, frame_id={self.frame_id}, obstacles={self.obstacles})"
        )

    def __repr__(self):
        return self.__str__()
