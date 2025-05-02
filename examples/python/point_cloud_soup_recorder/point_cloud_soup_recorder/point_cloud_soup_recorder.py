import os
import sys

import cv2
import numpy as np
import zmq

try:
    from zmq_msgs.point_cloud_soup import PointCloudSoup
    from zmq_msgs.topic_ports import SOUP_TOPIC
except ImportError:
    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../../zmq_msgs/python')))
    from zmq_msgs.point_cloud_soup import PointCloudSoup
    from zmq_msgs.topic_ports import SOUP_TOPIC


def write_ply_ascii(filename, points, colors):
    assert len(points) == len(colors), "Points and colors must be the same size"
    with open(filename, 'w') as out:
        out.write("ply\n")
        out.write("format ascii 1.0\n")
        out.write(f"element vertex {len(points)}\n")
        out.write("property float x\n")
        out.write("property float y\n")
        out.write("property float z\n")
        out.write("property uchar red\n")
        out.write("property uchar green\n")
        out.write("property uchar blue\n")
        out.write("end_header\n")
        for pt, rgb in zip(points, colors):
            out.write(f"{pt[0]} {pt[1]} {pt[2]} {int(rgb[2])} {int(rgb[1])} {int(rgb[0])}\n")


def write_ply_binary(filename, points, colors):
    assert len(points) == len(colors), "Points and colors must be the same size"
    alpha = np.zeros((len(points), 1), dtype=np.uint8)
    point_cloud = np.empty(len(points), dtype=[
        ('x', np.float32),
        ('y', np.float32),
        ('z', np.float32),
        ('red', np.uint8),
        ('green', np.uint8),
        ('blue', np.uint8),
        ('alpha', np.uint8)
    ])
    point_cloud['x'] = points[:, 0]
    point_cloud['y'] = points[:, 1]
    point_cloud['z'] = points[:, 2]
    point_cloud['blue'] = colors[:, 0]
    point_cloud['green'] = colors[:, 1]
    point_cloud['red'] = colors[:, 2]
    point_cloud['alpha'] = alpha[:, 0]
    with open(filename, 'wb') as out:
        out.write(b"ply\n")
        out.write(b"format binary_little_endian 1.0\n")
        out.write(f"element vertex {len(points)}\n".encode())
        out.write(b"property float x\n")
        out.write(b"property float y\n")
        out.write(b"property float z\n")
        out.write(b"property uchar red\n")
        out.write(b"property uchar green\n")
        out.write(b"property uchar blue\n")
        out.write(b"property uchar alpha\n")
        out.write(b"end_header\n")
        out.write(point_cloud.tobytes())


class PointCloudSoupRecorder:
    def __init__(self, endpoint, output_dir, ascii=False):
        self.context = zmq.Context(1)
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt(zmq.RCVHWM, 1)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint)
        self.output_dir = output_dir
        self.ascii = ascii
        self.last_frame_id = 0
        self.depth3d = None
        print(f"Subscribing to {endpoint}")
        os.makedirs(output_dir, exist_ok=True)

    def loop_once(self):
        msg = self.socket.recv()
        point_cloud_soup = PointCloudSoup()
        point_cloud_soup.read(msg)

        frame_id = point_cloud_soup.frame_id
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            print(
                f"{frame_id - self.last_frame_id - 1} frames dropped. Current frame ID: {frame_id}, last frame ID: {self.last_frame_id}")
        self.last_frame_id = frame_id

        disparity = point_cloud_soup.disparity.img
        rectified = point_cloud_soup.rectified.img
        disparity_scaled = disparity / np.float32(16)
        disparity_to_depth4x4 = point_cloud_soup.disparity_to_depth4x4.copy()

        # Rotation disparity to raw cam
        rotation_disparity_to_raw_cam = point_cloud_soup.rotation_disparity_to_raw_cam

        # Rotation world to raw cam
        rotation_world_to_raw_cam = point_cloud_soup.rotation_world_to_raw_cam

        # Compute disparity_to_rotated_depth4x4 (rotated Q matrix)
        rotation_disparity_to_world = rotation_world_to_raw_cam.T @ rotation_disparity_to_raw_cam
        rotation_disparity_to_world_4x4 = np.eye(4, dtype=np.float32)
        rotation_disparity_to_world_4x4[:3, :3] = rotation_disparity_to_world
        disparity_to_rotated_depth4x4 = rotation_disparity_to_world_4x4 @ disparity_to_depth4x4

        # Negate the last row of the Q-matrix
        disparity_to_rotated_depth4x4[3, :] *= -1

        if self.depth3d is None:
            self.depth3d = cv2.reprojectImageTo3D(disparity_scaled, disparity_to_rotated_depth4x4)
        else:
            cv2.reprojectImageTo3D(disparity_scaled, disparity_to_rotated_depth4x4, self.depth3d)

        xyz = self.depth3d
        bgr = rectified

        x = xyz[:, :, 0]
        y = xyz[:, :, 1]
        z = xyz[:, :, 2]
        valid = ~(np.isinf(x) | np.isinf(y) | np.isinf(z))

        xyz = xyz[valid]
        bgr = bgr[valid]

        downsample = 10
        xyz = xyz[::downsample, :]
        bgr = bgr[::downsample, :]

        total = disparity.size
        print(f"{len(xyz)} / {total} number of points used")
        print(f"{np.sum(valid)} / {total} valid points")
        filename = os.path.join(self.output_dir, f"{frame_id:09}.ply")
        print(f"\rWriting {filename}", flush=True)
        if bgr.dtype == np.uint16 or bgr.dtype == np.int16:
            bgr = (bgr / 257).astype(np.uint8)
        if self.ascii:
            write_ply_ascii(filename, xyz, bgr)
        else:
            write_ply_binary(filename, xyz, bgr)
        return


def print_usage(default_ip, default_output_dir):
    print("You should specify the IP address of the device running Hammerhead,\n"
          "as well as the folder where you want the data to be saved:\n\n"
          "     python point_cloud_soup_recorder.py hammerhead_ip output_dir\n\n"
          "e.g. python point_cloud_soup_recorder.py 192.168.1.9 point_clouds\n\n"
          "If unspecified, then we assume that you are running this on the device running Hammerhead,\n"
          "along with the other defaults\n\n"
          f"     python point_cloud_soup_recorder.py {default_ip} {default_output_dir}\n"
          "----------------------------------------")


def main():
    default_ip = "127.0.0.1"
    topic = SOUP_TOPIC
    default_output_dir = "point_clouds"
    if len(sys.argv) < 3:
        print_usage(default_ip, default_output_dir)
    ip = sys.argv[1] if len(sys.argv) > 1 else default_ip
    endpoint = f"tcp://{ip}:{topic.port}"

    output_dir = sys.argv[2] if len(sys.argv) >= 3 else default_output_dir
    point_cloud_soup_recorder = PointCloudSoupRecorder(endpoint, output_dir)
    index = 0
    while True:
        index += 1
        point_cloud_soup_recorder.loop_once()


if __name__ == "__main__":
    main()
