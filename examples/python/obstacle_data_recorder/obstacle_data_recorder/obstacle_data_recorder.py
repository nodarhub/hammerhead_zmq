import os
import sys

import zmq

try:
    from zmq_msgs.obstacle_data import ObstacleData
    from zmq_msgs.topic_ports import OBSTACLE_TOPIC
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.obstacle_data import ObstacleData
    from zmq_msgs.topic_ports import OBSTACLE_TOPIC


def write_data(filename, obstacle_data):
    with open(filename, "w") as out:
        out.write("x1,z1,x2,z2,x3,z3,x4,z4,vx,vz\n")
        for obstacle in obstacle_data.obstacles:
            for p in obstacle.bounding_box.points:
                out.write(f"{p.x:.6f},{p.z:.6f},")
            out.write(f"{obstacle.velocity.x:.6f},{obstacle.velocity.z:.6f}\n")


class ObstacleDataSink:
    def __init__(self, endpoint, output_dir):
        self.context = zmq.Context(1)
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt(zmq.RCVHWM, 1)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint)
        self.last_frame_id = 0
        print(f"Subscribing to {endpoint}")
        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)

    def loop_once(self):
        msg = self.socket.recv()
        obstacle_data = ObstacleData()
        obstacle_data.read(msg)
        frame_id = obstacle_data.frame_id

        # Warn if we dropped a frame
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            print(
                f"{frame_id - self.last_frame_id - 1} frames dropped. "
                f"Current frame ID: {frame_id}, last frame ID: {self.last_frame_id}"
            )
        self.last_frame_id = frame_id

        filename = os.path.join(self.output_dir, f"{frame_id:09}.txt")
        print(f"\rWriting {filename}", end="", flush=True)
        write_data(filename, obstacle_data)


def print_usage(default_ip, default_output_dir):
    print(
        "You should specify the IP address of the device running Hammerhead,\n"
        "as well as the folder where you want the data to be saved:\n\n"
        "     python obstacle_data_recorder.py hammerhead_ip output_dir\n\n"
        "e.g. python obstacle_data_recorder.py 192.168.1.9 obstacle_datas\n\n"
        "If unspecified, then we assume that you are running this on the device running Hammerhead,\n"
        "along with the other defaults\n\n"
        f"     python obstacle_data_recorder.py {default_ip} {default_output_dir}\n"
        "----------------------------------------"
    )


def main():
    default_ip = "127.0.0.1"
    topic = OBSTACLE_TOPIC
    default_output_dir = "obstacle_datas"
    if len(sys.argv) < 3:
        print_usage(default_ip, default_output_dir)
    ip = sys.argv[1] if len(sys.argv) > 1 else default_ip

    output_dir = sys.argv[2] if len(sys.argv) >= 3 else default_output_dir
    endpoint = f"tcp://{ip}:{topic.port}"
    sink = ObstacleDataSink(endpoint, output_dir)
    while True:
        sink.loop_once()


if __name__ == "__main__":
    main()
