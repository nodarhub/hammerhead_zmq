import argparse
import os
import signal
import struct
import sys
import time
import uuid
from datetime import datetime

import cv2
import numpy as np
import numpy.typing as npt
import yaml
import zmq

FRAME_RATE = 5

try:
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import get_reserved_ports
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import get_reserved_ports

# Global variable for signal handling
running = True

def signal_handler(signum, frame):
    global running
    print("\nSIGINT or SIGTERM received. Exiting...", file=sys.stderr)
    running = False

PIXEL_FORMAT_MAP = {
    "BGR": StampedImage.COLOR_CONVERSION.BGR2BGR,
    "Bayer_RGGB": cv2.COLOR_BayerBG2BGR,
    "Bayer_GRBG": cv2.COLOR_BayerGB2BGR,
    "Bayer_BGGR": cv2.COLOR_BayerRG2BGR,
    "Bayer_GBRG": cv2.COLOR_BayerGR2BGR,
}


class Publisher:
    def __init__(self, topic, ip, port):
        self.context = zmq.Context(1)
        self.socket = self.context.socket(zmq.PUB)
        self.socket.setsockopt(zmq.SNDHWM, 1)
        if ip:
            self.socket.connect(f"tcp://{ip}:{port}")
        else:
            self.socket.bind(f"tcp://*:{port}")

    def send(self, buffer):
        self.socket.send(buffer)


class TopbotPublisher:
    def __init__(self, port):
        self.publisher = Publisher("external/topbot_raw", "", port)
        time.sleep(1)

    def publish_image(
        self,
        img: npt.NDArray,
        timestamp: int,
        frame_id: int,
        cvt_to_bgr_code: StampedImage.COLOR_CONVERSION,
        extrinsics: dict | None = None,
    ):
        stamped_image = StampedImage(timestamp, frame_id, cvt_to_bgr_code, img)
        if extrinsics is not None:
            attached_extrinsics(
                stamped_image,
                extrinsics["euler_x_deg"],
                extrinsics["euler_y_deg"],
                extrinsics["euler_z_deg"],
                extrinsics["Tx"],
                extrinsics["Ty"],
                extrinsics["Tz"],
            )
        buffer = bytearray(stamped_image.msg_size())
        stamped_image.write(buffer, 0)
        self.publisher.send(buffer)
        return True


# Function to get files from a directory with a specific extension
def get_files(dirname, ext):
    files = [os.path.join(dirname, f) for f in os.listdir(dirname) if f.endswith(ext)]
    files.sort()
    return files


def is_valid_port(port):
    if port < 1024 or port > 65535:
        print("Invalid port number: Port number must be in the range [1024, 65535].")
        return False
    if port in get_reserved_ports():
        print(
            "Invalid port number: Port number is reserved. "
            "Please choose a port number other than 98xx."
        )
        return False
    return True


def parse_pixel_format(format_str):
    if format_str not in PIXEL_FORMAT_MAP:
        print(f"Unsupported pixel format: {format_str}")
        print(f"Supported formats: {', '.join(PIXEL_FORMAT_MAP.keys())}")
        return None
    return PIXEL_FORMAT_MAP[format_str]


def argument_parsing():
    parser = argparse.ArgumentParser(description="Topbot Publisher")
    parser.add_argument(
        "topbot_data_directory",
        help="Path to the topbot data directory"
    )
    parser.add_argument(
        "port_number",
        type=int,
        help="Port number to publish the data on"
    )
    parser.add_argument(
        "pixel_format",
        nargs="?",
        default="BGR",
        choices=PIXEL_FORMAT_MAP.keys(),
        help=(
            "Optional pixel format after loaded by OpenCV (default: BGR)."
            f" Valid choice: {list(PIXEL_FORMAT_MAP.keys())}"
        ),
        metavar="pixel_format",
    )
    parser.add_argument(
        "--extr",
        required=False,
        default=None,
        help="Optional path to the folder containing per-frame extrinsics files"
    )
    return parser.parse_args()


def get_one_extrinsiscs(image_file_path: str, extrinsics_dir: str) -> dict | None:
    if not os.path.exists(extrinsics_dir):
        print("The provided extrinsics directory does not exist:", extrinsics_dir)
        return None

    file_name_number_part = os.path.splitext(os.path.basename(image_file_path))[0]
    extrinsics_file = os.path.join(extrinsics_dir, file_name_number_part + ".yaml")
    if not os.path.exists(extrinsics_file):
        return None

    with open(extrinsics_file, 'r') as f:
        extrinsics = yaml.safe_load(f)
    for k in ["euler_x_deg", "euler_y_deg", "euler_z_deg", "Tx", "Ty", "Tz"]:
        if k not in extrinsics:
            print(f"Missing key '{k}' in extrinsics file: {extrinsics_file}")
            return None
    return extrinsics


def attached_extrinsics(
    stamped_image: StampedImage, euler_x_deg, euler_y_deg, euler_z_deg, tx, ty, tz
) -> None:
    stamped_image.additional_field = (
        uuid.UUID("2c5e9c77-a730-42ce-ac21-c33e26793bcb").bytes
        + struct.pack("=dddddd", euler_x_deg, euler_y_deg, euler_z_deg, tx, ty, tz)
    )


def main():
    global running
    args = argument_parsing()

    # Setup signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    image_dir = args.topbot_data_directory
    image_files = get_files(image_dir, ".tiff")
    if not image_files:
        print("No images found in folder.")
        return

    port = args.port_number
    if not is_valid_port(port):
        print("Invalid port number.")
        return

    # Parse pixel format
    cvt_to_bgr_code = PIXEL_FORMAT_MAP[args.pixel_format]
    if cvt_to_bgr_code is None:
        print("Invalid pixel format specified.")
        return

    extrinsics_dir = args.extr
    if extrinsics_dir:
        print("Using extrinsics from:", extrinsics_dir)

    publisher = TopbotPublisher(port)
    frame_id = 0

    for file in image_files:
        if not running:
            break
        # Load the image
        img = cv2.imread(file, cv2.IMREAD_UNCHANGED)
        if img is None:
            print(f"Failed to load image: {file}")
            continue

        # Check that the image is loaded in the format expected by cvt_to_bgr_code.
        # If using a different format, then add your checks
        if cvt_to_bgr_code == StampedImage.COLOR_CONVERSION.BGR2BGR:
            if img.ndim != 3 or img.shape[2] != 3:
                print(f"[ERROR] Image {file} does not have 3 channels for BGR2BGR")
                continue
            if img.dtype not in [np.uint8, np.uint16]:
                print(f"[ERROR] Unsupported dtype in {file}: {img.dtype}")
                continue
        elif cvt_to_bgr_code in [
            cv2.COLOR_BayerBG2BGR,
            cv2.COLOR_BayerGB2BGR,
            cv2.COLOR_BayerRG2BGR,
            cv2.COLOR_BayerGR2BGR,
        ]:
            if img.ndim != 2:
                print(f"[ERROR] Image {file} is expected to be single-channel for Bayer format")
                continue
            if img.dtype not in [np.uint8, np.uint16]:
                print(f"[ERROR] Unsupported dtype in {file}: {img.dtype}")
                continue

        # Check extrinsics if provided
        extrinsics = get_one_extrinsiscs(file, extrinsics_dir) if extrinsics_dir else None

        # Ready to publish
        timestamp = int(datetime.now().timestamp() * 1e9)
        if publisher.publish_image(img, timestamp, frame_id, cvt_to_bgr_code, extrinsics):
            print(
                f"Published frame {frame_id} from {file}"
                + (" with extrinsics" if extrinsics else "")
            )
            frame_id += 1
        time.sleep(1 / FRAME_RATE)
    
    print("Publisher stopped.")


if __name__ == "__main__":
    main()
