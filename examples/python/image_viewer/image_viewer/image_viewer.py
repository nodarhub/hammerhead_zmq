import os
import sys

import cv2
import numpy as np
import zmq

try:
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import IMAGE_TOPICS
except ImportError:
    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../../zmq_msgs/python')))
    from zmq_msgs.image import StampedImage
    from zmq_msgs.topic_ports import IMAGE_TOPICS

# Mapping of OpenCV types to strings
types = {
    0: "CV_8UC1",
    16: "CV_8UC3",
    1: "CV_8SC1",
    17: "CV_8SC3",
    2: "CV_16UC1",
    18: "CV_16UC3",
    3: "CV_16SC1",
    19: "CV_16SC3",
}


class ZMQImageViewer:
    def __init__(self, endpoint: str):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt(zmq.RCVHWM, 1)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint)
        self.window_name = endpoint
        self.last_frame_id = 0
        print(f"Subscribing to {endpoint}")
        cv2.namedWindow(self.window_name, cv2.WINDOW_NORMAL)

    def loop_once(self):
        msg = self.socket.recv()
        stamped_image = StampedImage()
        stamped_image.read(msg)
        img = stamped_image.img
        if img.dtype == np.int16:
            # Highgui produces a strange-looking output for signed 16-bit images. Convert to unsigned
            img = img.astype(np.uint16)
        if img is None or img.size == 0:
            print("img is None or img.size == 0")
            return True

        frame_id = stamped_image.frame_id
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            print(
                f"{frame_id - self.last_frame_id - 1} frames dropped. Current frame ID: {frame_id}, last frame ID: {self.last_frame_id}")
        self.last_frame_id = frame_id
        print(f"\rFrame # {frame_id}, img.shape = {img.shape}, img.dtype = {img.dtype}", end="", flush=True)

        cv2.resizeWindow(self.window_name, 640, 480)
        cv2.imshow(self.window_name, img)
        cv2.waitKey(1)

        stop_on_close = False
        if stop_on_close and not cv2.getWindowProperty(self.window_name, cv2.WND_PROP_VISIBLE):
            print("Stopping...")
            return False
        return True


DEFAULT_IP = "127.0.0.1"
DEFAULT_PORT = "9800"
DEFAULT_TOPIC = IMAGE_TOPICS[0]


def print_usage(DEFAULT_IP, DEFAULT_PORT):
    print("You should specify the Orin's IP address as well as\n"
          "the port of the message that you want to listen to like this:\n\n"
          "     python image_viewer.py orin_ip port\n\n"
          "e.g. python image_viewer.py 192.168.1.9 9800\n\n"
          "Alternatively, you can specify one of the image topic names provided in topic_ports.hpp of zmq_msgs:"
          "e.g. python image_viewer.py 192.168.1.9 nodar/right/image_raw\n\n"
          "In the meantime, we are going to assume that you are running this on the Orin itself,\n"
          "and that you want the images on port 9800, that is, we assume that you specified\n\n"
          f"     python image_viewer.py {DEFAULT_IP} {DEFAULT_PORT}\n\n"
          "Note that the list of topic/port mappings is in topic_ports.hpp header in the zmq_msgs target.\n"
          "----------------------------------------")


def main():
    DEFAULT_IP = "127.0.0.1"
    DEFAULT_TOPIC = IMAGE_TOPICS[0]
    DEFAULT_PORT = DEFAULT_TOPIC.port
    if len(sys.argv) < 3:
        print_usage(DEFAULT_IP, DEFAULT_PORT)

    ip = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_IP

    topic = DEFAULT_TOPIC
    if len(sys.argv) >= 3:
        try:
            port = int(sys.argv[2])
            for image_topic in IMAGE_TOPICS:
                if port == image_topic.port:
                    topic = image_topic
                    break
            else:
                print(
                    f"It seems like you specified a port number {port} that does not correspond to a port on which images are being published.")
                return
        except ValueError:
            topic_name = sys.argv[2]
            for image_topic in IMAGE_TOPICS:
                if topic_name == image_topic.name:
                    topic = image_topic
                    break
            else:
                print(
                    f"It seems like you specified a topic name {topic_name} that does not correspond to a topic on which images are being published.")
                return

    endpoint = f"tcp://{ip}:{topic.port}"
    subscriber = ZMQImageViewer(endpoint)
    running = True
    try:
        while running:
            subscriber.loop_once()
    except KeyboardInterrupt:
        print("\nExiting...")


if __name__ == "__main__":
    main()
