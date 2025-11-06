import os
import struct
import sys
import time
from collections import deque
from datetime import datetime

import cv2
import zmq

try:
    import tifffile
    HAS_TIFFFILE = True
except ImportError:
    HAS_TIFFFILE = False

try:
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import *
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import *

folder_name_dict = {
    LEFT_RAW_TOPIC.name: "left_raw",
    RIGHT_RAW_TOPIC.name: "right_raw",
    LEFT_RECT_TOPIC.name: "left_rect",
    RIGHT_RECT_TOPIC.name: "right_rect",
    DISPARITY_TOPIC.name: "disparity",
    COLOR_BLENDED_DEPTH_TOPIC.name: "color_blended_depth",
    TOPBOT_RAW_TOPIC.name: "topbot_raw",
    TOPBOT_RECT_TOPIC.name: "topbot_rect",
}


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


class ZMQImageRecorder:
    def __init__(self, endpoint, output_dir, image_dirname):
        self.context = zmq.Context(1)
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt(zmq.RCVHWM, 1)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint)
        self.last_frame_id = 0
        print(f"Subscribing to {endpoint}")
        self.image_dir = output_dir + "/" + image_dirname
        self.timing_dir = output_dir + "/times"
        print(f"Creating the directory {self.image_dir}")
        os.makedirs(self.image_dir, exist_ok=True)
        print(f"Creating the directory {self.timing_dir}")
        os.makedirs(self.timing_dir, exist_ok=True)
        # Create a separate timing file with all times
        self.timing_file = open(output_dir + "/times.txt", "w")
        # Create a file to log how long each loop_once call is taking.
        # This helps to identify any bottlenecks in SSD write performance
        self.loop_latency_log = open(output_dir + "/loop_latency.txt", "w")
        # This means NO COMPRESSION in libtiff
        self.compression_params = [cv2.IMWRITE_TIFF_COMPRESSION, 1]
        self.fps = FPS()

    @staticmethod
    def add_tiff_metadata(filename, left_time, right_time):
        """Add timestamp metadata to TIFF file (compatible with Hammerhead viewer format)"""
        if not HAS_TIFFFILE:
            return  # Skip if tifffile not available

        # Create YAML string with all required fields (compatible with Hammerhead format)
        # Must include all fields that DetailsParameters expects
        details_str = (
            f"left_time: {left_time}\n"
            f"right_time: {right_time}\n"
            f"focal_length: 0.0\n"
            f"baseline: 0.0\n"
            f"meters_above_ground: 0.0\n"
            f"projection: [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]\n"
            f"rotation_disparity_to_raw_cam: [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]\n"
            f"rotation_world_to_raw_cam: [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]\n"
        )
        metadata_yaml = f'details: "{details_str}"'

        # Use tifffile to add metadata to the SOFTWARE tag (tag 305)
        try:
            with tifffile.TiffFile(filename) as tif:
                img_data = tif.asarray()
                # Write back with metadata in SOFTWARE tag (305)
                tifffile.imwrite(
                    filename,
                    img_data,
                    compression=None,
                    metadata=None,
                    extratags=[(305, 's', 0, metadata_yaml, True)]  # TIFFTAG_SOFTWARE
                )
        except Exception:
            # If adding metadata fails, skip it (metadata is optional)
            pass

    def loop_once(self):
        loop_start_time = time.perf_counter()
        self.fps.tic()
        msg = self.socket.recv()
        stamped_image = StampedImage()
        stamped_image.read(msg)
        img = stamped_image.img
        if img is None or img.size == 0:
            print("img is None or img.size == 0")
            return

        frame_id = stamped_image.frame_id
        info_str = f"Frame # {frame_id}, Last #: {self.last_frame_id}, img.shape = {img.shape}, img.dtype = {img.dtype}. "
        fps_str = f"{self.fps}. "
        drop_str = ""
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            drop_str = f"Frames dropped: {frame_id - self.last_frame_id - 1}. "
        print(f"\r{info_str}{fps_str}{drop_str}", end="", flush=True)
        self.last_frame_id = frame_id

        # We recommend saving tiffs with no compression if the data rate is high.
        # Depending on the underlying image type, consider using stamped_image.cvt_to_bgr_code
        # to convert to BGR before saving.
        tiff_path = self.image_dir + f"/{frame_id:09}.tiff"
        cv2.imwrite(tiff_path, img, self.compression_params)

        # Extract right_time from additional_field if present (for topbot messages)
        right_time = None
        if len(stamped_image.additional_field) == 8:
            right_time = struct.unpack('<Q', stamped_image.additional_field)[0]

        # Add timestamp metadata to TIFF file (for topbot images with dual timestamps)
        if right_time is not None:
            self.add_tiff_metadata(tiff_path, stamped_image.time, right_time)

        with open(self.timing_dir + f"/{frame_id:09}.txt", "w") as f:
            f.write(f"{stamped_image.time}")
            if right_time is not None:
                f.write(f" {right_time}")
            f.write("\n")

        self.timing_file.write(f"{frame_id:09} {stamped_image.time}")
        if right_time is not None:
            self.timing_file.write(f" {right_time}")
        self.timing_file.write("\n")
        self.timing_file.flush()
        loop_stop_time = time.perf_counter()
        self.loop_latency_log.write(f"{frame_id:09} {loop_stop_time - loop_start_time}\n")
        self.loop_latency_log.flush()
        return

    def close(self):
        self.timing_file.close()
        self.loop_latency_log.close()


def print_usage(default_ip, default_port, default_output_dir):
    print(
        "You should specify the IP address of the ZMQ source (the device running Hammerhead), \n"
        "the port number of the message that you want to listen to, \n"
        "and the folder where you want the data to be saved:\n\n"
        "     python image_recorder.py hammerhead_ip port output_dir\n\n"
        "e.g. python image_recorder.py 192.168.1.9 9800 recorded_images\n\n"
        "Alternatively, you can specify one of the image topic names in topic_ports.py of zmq_msgs...\n\n"
        "e.g. python image_recorder.py 192.168.1.9 nodar/right/image_raw recorded_images\n\n"
        "If unspecified, we assume you are running this on the device running Hammerhead,\n"
        "along with the defaults\n\n"
        f"     python image_recorder.py {default_ip} {default_port} {default_output_dir}\n\n"
        "Note that the list of topic/port mappings is in topic_ports.py in the zmq_msgs target.\n"
        "----------------------------------------"
    )


def main():
    default_ip = "127.0.0.1"
    default_topic = IMAGE_TOPICS[0]
    default_port = default_topic.port
    dated_folder = datetime.now().strftime("%Y%m%d-%H%M%S")
    default_output_dir = "./" + dated_folder

    if len(sys.argv) < 4:
        print_usage(default_ip, default_port, default_output_dir)

    ip = sys.argv[1] if len(sys.argv) > 1 else default_ip

    topic = default_topic
    if len(sys.argv) >= 3:
        try:
            port = int(sys.argv[2])
            for image_topic in IMAGE_TOPICS:
                if port == image_topic.port:
                    topic = image_topic
                    break
            else:
                print(
                    f"It seems like you specified a port number {port} "
                    "that does not correspond to a port on which images are being published."
                )
                return
        except ValueError:
            topic_name = sys.argv[2]
            for image_topic in IMAGE_TOPICS:
                if topic_name == image_topic.name:
                    topic = image_topic
                    break
            else:
                print(
                    f"It seems like you specified a topic name {topic_name} "
                    "that does not correspond to a topic on which images are being published."
                )
                return

    output_dir = (sys.argv[3] + "/" + dated_folder) if len(sys.argv) >= 4 else default_output_dir
    endpoint = f"tcp://{ip}:{topic.port}"
    subscriber = ZMQImageRecorder(endpoint, output_dir, folder_name_dict.get(topic.name, "images"))
    try:
        while True:
            subscriber.loop_once()
    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        subscriber.close()


if __name__ == "__main__":
    main()
