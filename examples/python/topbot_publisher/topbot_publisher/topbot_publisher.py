import os
import sys
import time
from datetime import datetime

import cv2
import numpy as np
import zmq

FRAME_RATE = 10

try:
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import IMAGE_TOPICS
except ImportError:
    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../../zmq_msgs/python')))
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import get_reserved_ports


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

    def publish_image(self, img, timestamp, frame_id):
        if img is None:
            print("Failed to load image")
            return False
        if img.ndim != 3:
            print("Hammerhead will be expected 3-channel images")
            return
        if not (img.dtype == np.uint8 or img.dtype == np.uint16):
            print(f"Hammerhead expects either uint8 or uint16 pixels")
            return False
        buffer = bytearray(StampedImage.msg_size(img.shape[0], img.shape[1], img.shape[2], img.dtype))
        StampedImage.writes(buffer, 0, timestamp, frame_id, img)
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


def main():
    if len(sys.argv) < 3:
        print("Usage: topbot_publisher <topbot_data_directory> <port_number>")
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

    publisher = TopbotPublisher(port)
    frame_id = 0
    while True:
        for file in image_files:
            img = cv2.imread(file, cv2.IMREAD_UNCHANGED)
            timestamp = int(datetime.now().timestamp() * 1e9)
            if publisher.publish_image(img, timestamp, frame_id):
                print(f"Published frame {frame_id} from {file}")
                frame_id += 1
            time.sleep(1 / FRAME_RATE)

    print("Publisher stopped.")


if __name__ == "__main__":
    main()
