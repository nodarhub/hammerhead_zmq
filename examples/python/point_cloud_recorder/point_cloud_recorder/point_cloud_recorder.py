import os
import sys

import zmq

try:
    from zmq_msgs.point_cloud import PointCloud
    from zmq_msgs.topic_ports import POINT_CLOUD_TOPIC
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.point_cloud import PointCloud
    from zmq_msgs.topic_ports import POINT_CLOUD_TOPIC


def writePlyAscii(filename, points):
    with open(filename, "w") as out:
        out.write("ply\n")
        out.write("format ascii 1.0\n")
        out.write(f"element vertex {len(points)}\n")
        out.write("property float x\n")
        out.write("property float y\n")
        out.write("property float z\n")
        out.write("end_header\n")
        for pt in points:
            out.write(f"{pt[0]} {pt[1]} {pt[2]}\n")


def writePlyBinary(filename, points):
    with open(filename, "wb") as out:
        out.write(b"ply\n")
        out.write(b"format binary_little_endian 1.0\n")
        out.write(f"element vertex {len(points)}\n".encode())
        out.write(b"property float x\n")
        out.write(b"property float y\n")
        out.write(b"property float z\n")
        out.write(b"end_header\n")
        out.write(points.tobytes())


class PointCloudRecorder:
    def __init__(self, endpoint, output_dir, ascii=False):
        self.context = zmq.Context(1)
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt(zmq.RCVHWM, 1)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint)
        self.last_frame_id = 0
        print(f"Subscribing to {endpoint}")
        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)
        self.ascii = ascii

    def loop_once(self):
        msg = self.socket.recv()
        point_cloud = PointCloud()
        point_cloud.read(msg)
        if point_cloud.points is None or point_cloud.points.size == 0:
            print("point_cloud is None or point_cloud.size == 0")
            return

        frame_id = point_cloud.frame_id
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            print(
                f"{frame_id - self.last_frame_id - 1} frames dropped. Current frame ID: {frame_id}, last frame ID: {self.last_frame_id}"
            )
        self.last_frame_id = frame_id

        filename = os.path.join(self.output_dir, f"{frame_id:09}.ply")
        print(f"\rWriting {filename}", end="", flush=True)
        if self.ascii:
            writePlyAscii(filename, point_cloud.points)
        else:
            writePlyBinary(filename, point_cloud.points)
        return


def print_usage(default_ip, default_output_dir):
    print(
        "You should specify the IP address of the device running Hammerhead,\n"
        "as well as the folder where you want the data to be saved:\n\n"
        "     python point_cloud_recorder.py hammerhead_ip output_dir\n\n"
        "e.g. python point_cloud_recorder.py 192.168.1.9 point_clouds\n\n"
        "If unspecified, then we assume that you are running this on the device running Hammerhead,\n"
        "along with the other defaults\n\n"
        f"     python point_cloud_recorder.py {default_ip} {default_output_dir}\n"
        "----------------------------------------"
    )


def main():
    default_ip = "127.0.0.1"
    topic = POINT_CLOUD_TOPIC
    default_output_dir = "point_clouds"
    if len(sys.argv) < 3:
        print_usage(default_ip, default_output_dir)
    ip = sys.argv[1] if len(sys.argv) > 1 else default_ip

    output_dir = sys.argv[2] if len(sys.argv) >= 3 else default_output_dir
    endpoint = f"tcp://{ip}:{topic.port}"
    subscriber = PointCloudRecorder(endpoint, output_dir)
    while True:
        subscriber.loop_once()


if __name__ == "__main__":
    main()
