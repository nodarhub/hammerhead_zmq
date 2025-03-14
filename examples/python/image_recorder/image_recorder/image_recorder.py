import os
import sys

import cv2
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


class ZMQImageRecorder:
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
        self.compression_params = [cv2.IMWRITE_TIFF_COMPRESSION, 1]

    def loop_once(self):
        msg = self.socket.recv()
        stamped_image = StampedImage()
        stamped_image.read(msg)
        img = stamped_image.img
        if img is None or img.size == 0:
            print("img is None or img.size == 0")
            return

        frame_id = stamped_image.frame_id
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            print(
                f"{frame_id - self.last_frame_id - 1} frames dropped. Current frame ID: {frame_id}, last frame ID: {self.last_frame_id}")
        self.last_frame_id = frame_id
        print(f"\rFrame # {frame_id}, img.shape = {img.shape}, img.dtype = {img.dtype}", end="", flush=True)

        # We recommend saving tiffs with no compression if the data rate is high.
        cv2.imwrite(self.output_dir + f"/{frame_id:09}.tiff", img, self.compression_params)
        return


def print_usage(default_ip, default_port, default_output_dir):
    print("You should specify the IP address of the ZMQ source (the device running Hammerhead),\n"
          "the topic name or port number, and the folder where you want the data to be saved:\n\n"
          "     ./image_recorder orin_ip port output_dir\n\n"
          "e.g. ./image_recorder 192.168.1.9 9800 images\n\n"
          "Alternatively, you can specify one of the image topic names provided in topic_ports.hpp of zmq_msgs:"
          "e.g. ./image_recorder 192.168.1.9 nodar/right/image_raw recorded_images\n\n"
          "If unspecified, then we assume that you are running this on the device running Hammerhead,\n"
          "along with the other defaults\n\n"
          f"     ./image_recorder {default_ip} {default_port} {default_output_dir}\n\n"
          "Note that the list of topic/port mappings is in topic_ports.hpp header in the zmq_msgs target.\n"
          "----------------------------------------")


def main():
    default_ip = "127.0.0.1"
    default_topic = IMAGE_TOPICS[0]
    default_port = default_topic.port
    default_output_dir = "recorded_images"
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

    output_dir = sys.argv[3] if len(sys.argv) >= 4 else default_output_dir
    endpoint = f"tcp://{ip}:{topic.port}"
    subscriber = ZMQImageRecorder(endpoint, output_dir)
    while True:
        subscriber.loop_once()


if __name__ == "__main__":
    main()
