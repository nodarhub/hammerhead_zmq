import cv2
import numpy as np
import os
import sys
import time
import zmq
from datetime import datetime

FRAME_RATE = 10

try:
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import get_reserved_ports
except ImportError:
    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../../zmq_msgs/python')))
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import get_reserved_ports

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

    def publish_image(self, img, timestamp, frame_id, cvt_to_bgr_code):
        stamped_image = StampedImage(timestamp, frame_id, cvt_to_bgr_code, img)
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
        print("Invalid port number: Port number is reserved. Please choose a port number other than 98xx.")
        return False
    return True


def parse_pixel_format(format_str):
    if format_str not in PIXEL_FORMAT_MAP:
        print(f"Unsupported pixel format: {format_str}")
        print(f"Supported formats: {', '.join(PIXEL_FORMAT_MAP.keys())}")
        return None
    return PIXEL_FORMAT_MAP[format_str]


def main():
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: topbot_publisher <topbot_data_directory> <port_number> [pixel_format]")
        print("Supported pixel formats: " + ', '.join(PIXEL_FORMAT_MAP.keys()))
        return

    image_dir = sys.argv[1]
    image_files = get_files(image_dir, ".tiff")
    if not image_files:
        print("No images found in folder.")
        return

    try:
        port = int(sys.argv[2])
    except ValueError:
        print("Invalid port number. Please provide an integer value.")
        return
    if not is_valid_port(port):
        return

    # Parse pixel format
    cvt_to_bgr_code = PIXEL_FORMAT_MAP["BGR"]
    if len(sys.argv) == 4:
        pixel_format = sys.argv[3]
        cvt_to_bgr_code = parse_pixel_format(pixel_format)
        if cvt_to_bgr_code is None:
            return

    publisher = TopbotPublisher(port)
    frame_id = 0
    while True:
        for file in image_files:
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
            elif cvt_to_bgr_code in [cv2.COLOR_BayerBG2BGR, cv2.COLOR_BayerGB2BGR,
                                     cv2.COLOR_BayerRG2BGR, cv2.COLOR_BayerGR2BGR]:
                if img.ndim != 2:
                    print(f"[ERROR] Image {file} is expected to be single-channel for Bayer format")
                    continue
                if img.dtype not in [np.uint8, np.uint16]:
                    print(f"[ERROR] Unsupported dtype in {file}: {img.dtype}")
                    continue

            # Ready to publish
            timestamp = int(datetime.now().timestamp() * 1e9)
            if publisher.publish_image(img, timestamp, frame_id, cvt_to_bgr_code):
                print(f"Published frame {frame_id} from {file}")
                frame_id += 1
            time.sleep(1 / FRAME_RATE)


if __name__ == "__main__":
    main()
