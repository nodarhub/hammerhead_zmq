import argparse
import os
import queue
import signal
import sys
import threading
import time
from collections import deque
from datetime import datetime

import numpy as np
import yaml
import zmq

import tifffile

try:
    from zmq_msgs.point_cloud_soup import PointCloudSoup
    from zmq_msgs.topic_ports import SOUP_TOPIC, WAIT_TOPIC
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.point_cloud_soup import PointCloudSoup
    from zmq_msgs.topic_ports import SOUP_TOPIC, WAIT_TOPIC


def write_ply_binary(filename, points, colors):
    assert len(points) == len(colors), "Points and colors must be the same size"
    point_cloud = np.empty(
        len(points),
        dtype=[("xyz", np.float32, 3), ("rgba", np.uint8, 4)],
    )
    point_cloud["xyz"] = points
    point_cloud["rgba"][:, 0] = colors[:, 2]
    point_cloud["rgba"][:, 1] = colors[:, 1]
    point_cloud["rgba"][:, 2] = colors[:, 0]
    point_cloud["rgba"][:, 3] = 1
    with open(filename, "wb") as out:
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
        point_cloud.tofile(out)


def write_details_yaml(filename, soup):
    """Write the soup's non-image fields as YAML.
    """
    details = {
        "left_time": int(soup.time),
        "right_time": 0,
        "exposure": 0.0,
        "gain": 0.0,
        "focal_length": float(soup.focal_length),
        "baseline": float(soup.baseline),
        "meters_above_ground": 0.0,
        "projection": [float(x) for x in np.asarray(soup.disparity_to_depth4x4).flatten()],
        "rotation_disparity_to_raw_cam": [
            float(x) for x in np.asarray(soup.rotation_disparity_to_raw_cam).flatten()
        ],
        "rotation_world_to_raw_cam": [
            float(x) for x in np.asarray(soup.rotation_world_to_raw_cam).flatten()
        ],
    }
    with open(filename, "w") as f:
        yaml.safe_dump(details, f, default_flow_style=None, sort_keys=False)


# Global variable for signal handling
running = True


def signal_handler(signum, frame):
    global running
    print("\nSIGINT or SIGTERM received.", file=sys.stderr)
    running = False


class FPS:
    def __init__(self):
        # Store timestamps for up to 100 frames
        self.frame_times = deque(maxlen=100)
        self.start_time = None
        self.total_frames = 0

    def tic(self):
        """Call this once per frame."""
        now = time.perf_counter()
        if self.start_time is None:
            self.start_time = now
        self.frame_times.append(now)
        self.total_frames += 1

    def fps_over_window(self, n):
        """Compute FPS over the last n frames (if enough data)."""
        if len(self.frame_times) < 2:
            return 0.0
        if len(self.frame_times) < n:
            n = len(self.frame_times)
        t1 = self.frame_times[-n]
        t2 = self.frame_times[-1]
        return (n - 1) / (t2 - t1) if (t2 - t1) > 0 else 0.0

    def __str__(self):
        fps_100 = self.fps_over_window(100)
        if self.total_frames > 1:
            total_time = self.frame_times[-1] - self.start_time
            fps_life = (self.total_frames - 1) / total_time if total_time > 0 else 0.0
        else:
            fps_life = 0.0
        return f"fps_100: {fps_100:.2f}, fps_inf: {fps_life:.2f}"


class PointCloudSoupRecorder:
    def __init__(
            self,
            endpoint,
            output_dir,
            output_format="ply",
            downsample=1,
            scheduler_endpoint=None,
    ):
        if output_format not in ("ply", "raw"):
            raise ValueError(f"Unknown output_format: {output_format!r}")
        self.context = zmq.Context(1)
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt(zmq.RCVHWM, 1)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint)

        self.scheduler_socket = None
        if scheduler_endpoint is not None:
            self.scheduler_socket = self.context.socket(zmq.DEALER)
            self.scheduler_socket.connect(scheduler_endpoint)
            print(f"Connected to scheduler at {scheduler_endpoint}")
        self.output_dir = output_dir
        self.output_format = output_format
        self.downsample = max(1, int(downsample))
        self.last_frame_id = 0
        self.fps = FPS()
        self._u_row = None
        self._v_col = None
        self._write_queue = None
        self._writer = None
        print(f"Subscribing to {endpoint}")

        self.details_dir = os.path.join(output_dir, "details")
        os.makedirs(self.details_dir, exist_ok=True)
        if output_format == "raw":
            self.disparity_dir = os.path.join(output_dir, "disparity")
            self.left_rect_dir = os.path.join(output_dir, "left-rect")
            os.makedirs(self.disparity_dir, exist_ok=True)
            os.makedirs(self.left_rect_dir, exist_ok=True)
        else:
            self.point_cloud_dir = os.path.join(output_dir, "point_clouds")
            os.makedirs(self.point_cloud_dir, exist_ok=True)
            if self.downsample == 1:
                print(
                    "NOTE: Python code is slow and you may see dropped frames in your "
                    "system. If you are seeing lower performance, please consider using "
                    "the C++ example."
                )
            # Persistent background writer.
            self._writer = threading.Thread(target=self._writer_loop)
            self._writer.start()

    def loop_once(self):
        self.fps.tic()
        msg = self.socket.recv()
        point_cloud_soup = PointCloudSoup()
        point_cloud_soup.read(msg)

        frame_id = point_cloud_soup.frame_id
        info_str = f"Frame # {frame_id}, Last #: {self.last_frame_id}, format = {self.output_format}. "
        fps_str = f"{self.fps}. "
        drop_str = ""
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            drop_str = f"Frames dropped: {frame_id - self.last_frame_id - 1}. "
        print(f"\r{info_str}{fps_str}{drop_str}", end="", flush=True)
        self.last_frame_id = frame_id

        details_path = os.path.join(self.details_dir, f"{frame_id:09}.yaml")
        write_details_yaml(details_path, point_cloud_soup)

        if self.output_format == "raw":
            self._write_raw(point_cloud_soup, frame_id)
        else:
            self._write_point_cloud(point_cloud_soup, frame_id)

        # Wait for scheduler request from Hammerhead, then reply so Hammerhead
        # can produce the next frame (only active when --wait-for-scheduler).
        if self.scheduler_socket is not None:
            self.scheduler_socket.recv()  # blocking
            self.scheduler_socket.send(b"OK")

    def _write_raw(self, soup, frame_id):
        disparity = soup.disparity.img
        rectified = soup.rectified.img
        if disparity.ndim == 3 and disparity.shape[2] == 1:
            disparity = disparity[:, :, 0]
        if disparity.dtype != np.uint16:
            disparity = np.clip(disparity, 0, None).astype(np.uint16)
        disparity_path = os.path.join(self.disparity_dir, f"{frame_id:09}.tiff")
        left_rect_path = os.path.join(self.left_rect_dir, f"{frame_id:09}.tiff")
        tifffile.imwrite(disparity_path, disparity, compression="none")
        tifffile.imwrite(left_rect_path, rectified[:, :, ::-1], compression="none")

    def _write_point_cloud(self, point_cloud_soup, frame_id):
        disparity = point_cloud_soup.disparity.img
        rectified = point_cloud_soup.rectified.img
        if disparity.ndim == 3 and disparity.shape[2] == 1:
            disparity = disparity[:, :, 0]
        disparity_to_depth4x4 = point_cloud_soup.disparity_to_depth4x4.copy()
        rotation_disparity_to_raw_cam = point_cloud_soup.rotation_disparity_to_raw_cam
        rotation_world_to_raw_cam = point_cloud_soup.rotation_world_to_raw_cam
        rotation_disparity_to_world = rotation_world_to_raw_cam.T @ rotation_disparity_to_raw_cam
        rotation_disparity_to_world_4x4 = np.eye(4, dtype=np.float32)
        rotation_disparity_to_world_4x4[:3, :3] = rotation_disparity_to_world
        Q = (rotation_disparity_to_world_4x4 @ disparity_to_depth4x4).astype(np.float32)
        Q[3, :] *= -1

        H, W = disparity.shape
        if self._u_row is None or self._u_row.shape[0] != W:
            self._u_row = np.arange(W, dtype=np.float32)
            self._v_col = np.arange(H, dtype=np.float32)

        flat = np.flatnonzero(disparity)
        if self.downsample > 1:
            flat = flat[:: self.downsample]
        vs = flat // W
        us = flat % W
        u_f = self._u_row[us]
        v_f = self._v_col[vs]
        d = disparity.ravel()[flat] * np.float32(1.0 / 16.0)
        
        w = Q[3, 0] * u_f + Q[3, 1] * v_f + Q[3, 2] * d + Q[3, 3]
        w_valid = w != 0
        if not w_valid.all():
            vs = vs[w_valid]
            us = us[w_valid]
            u_f = u_f[w_valid]
            v_f = v_f[w_valid]
            d = d[w_valid]
            w = w[w_valid]
        inv_w = np.float32(1.0) / w
        x = (Q[0, 0] * u_f + Q[0, 1] * v_f + Q[0, 2] * d + Q[0, 3]) * inv_w
        y = (Q[1, 0] * u_f + Q[1, 1] * v_f + Q[1, 2] * d + Q[1, 3]) * inv_w
        z = (Q[2, 0] * u_f + Q[2, 1] * v_f + Q[2, 2] * d + Q[2, 3]) * inv_w
        xyz = np.stack([x, y, z], axis=-1)
        bgr = rectified[vs, us]

        filename = os.path.join(self.point_cloud_dir, f"{frame_id:09}.ply")
        if bgr.dtype == np.uint16 or bgr.dtype == np.int16:
            bgr = (bgr / 257).astype(np.uint8)
        # Blocks if the worker hasn't drained the previous frame
        self._write_queue.put((filename, xyz, bgr))

    def _writer_loop(self):
        while True:
            item = self._write_queue.get()
            if item is None:
                return
            filename, xyz, bgr = item
            write_ply_binary(filename, xyz, bgr)

    def close(self):
        if self._writer is not None:
            self._write_queue.put(None)
            self._writer.join()
            self._writer = None


def build_arg_parser(default_ip):
    parser = argparse.ArgumentParser(
        description=(
            "Record PointCloudSoup messages from Hammerhead.\n\n"
            "Two output formats are available:\n"
            "  ply (default): reconstruct a 3D point cloud on this machine and write\n"
            "                 one {frame_id}.ply per message. A details/{frame_id}.yaml\n"
            "                 is also written so timestamps, baseline, focal length, and\n"
            "                 projection matrices are preserved.\n"
            "  raw          : skip reconstruction. Write the disparity and left rectified\n"
            "                 images plus details/{frame_id}.yaml into three sibling folders\n"
            "                 (disparity/, left-rect/, details/). Lets a downstream process\n"
            "                 do the 3D reconstruction offline, which keeps CPU usage low\n"
            "                 on the recording host."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--ip",
        default=default_ip,
        help=f"IP address of the device running Hammerhead (default: {default_ip}).",
    )
    parser.add_argument(
        "--output-dir",
        default=".",
        help=(
            "Base directory to write output into (default: current working directory). "
            "A UTC-timestamped subfolder (YYYYMMDD-HHMMSS) is appended so each run "
            "lands in its own directory."
        ),
    )
    parser.add_argument(
        "-f",
        "--format",
        choices=("ply", "raw"),
        default="ply",
        help=(
            "Output format. 'ply' reconstructs points on this machine; "
            "'raw' writes disparity + left-rect + details for offline reconstruction."
        ),
    )
    parser.add_argument(
        "--downsample",
        type=int,
        default=1,
        help=(
            "[ply format only] Keep every Nth valid point when writing the PLY. "
            "Writing every point can outpace disk bandwidth at high frame rates and "
            "large image sizes; increase if you see dropped frames. Has no effect "
            "when --format raw. Default: 1 (keep all points)."
        ),
    )
    parser.add_argument(
        "-w",
        "--wait-for-scheduler",
        action="store_true",
        help=(
            "Enable scheduler synchronization with Hammerhead. When enabled, the recorder "
            "waits for Hammerhead to request the next frame before continuing, ensuring "
            "frame-by-frame synchronization. Requires `wait_for_scheduler = 1` in "
            "Hammerhead's master_config.ini."
        ),
    )
    return parser


def main():
    global running
    default_ip = "127.0.0.1"
    topic = SOUP_TOPIC

    # Setup signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    args = build_arg_parser(default_ip).parse_args()

    endpoint = f"tcp://{args.ip}:{topic.port}"
    scheduler_endpoint = (
        f"tcp://{args.ip}:{WAIT_TOPIC.port}" if args.wait_for_scheduler else None
    )

    dated_folder = datetime.utcnow().strftime("%Y%m%d-%H%M%S")
    output_dir = os.path.join(args.output_dir, dated_folder)

    os.makedirs(output_dir, exist_ok=True)
    print(f"Writing output to {output_dir}")

    recorder = PointCloudSoupRecorder(
        endpoint,
        output_dir,
        output_format=args.format,
        downsample=args.downsample,
        scheduler_endpoint=scheduler_endpoint,
    )
    while running:
        recorder.loop_once()
    recorder.close()


if __name__ == "__main__":
    main()
