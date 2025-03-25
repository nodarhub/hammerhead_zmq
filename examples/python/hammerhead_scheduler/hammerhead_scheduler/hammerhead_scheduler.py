import struct
import sys
import time

import zmq


class Scheduler:
    def __init__(self, endpoint):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.ROUTER)
        self.socket.connect(endpoint)
        print(f"Subscribing to {endpoint}")
        self.last_frame_id = 0
        self.initialized = False

    def loop(self):
        # Wait for a scheduler request from hammerhead indicating that a frame is ready.
        identity_msg = self.socket.recv()
        request_msg = self.socket.recv()

        if len(request_msg) != 8:
            print("The scheduler request doesn't seem to be the right size.")
            return

        # Deserialize the request to get the frame id
        # https://docs.python.org/3/library/struct.html#format-characters
        frame_id = struct.unpack('Q', request_msg)[0]
        print(f"\rGot a scheduler request for frame {frame_id}", end='', flush=True)
        if self.initialized and frame_id != self.last_frame_id + 1:
            print(f"\nIt looks like we might be dropping frames\n"
                  f"Current frame_id = {frame_id}\n"
                  f"Last    frame_id = {self.last_frame_id}")

        self.initialized = True
        self.last_frame_id = frame_id

        # Halt hammerhead execution before telling it to continue
        time.sleep(0.5)
        reply = b''  # Intentionally empty message
        self.socket.send(identity_msg, zmq.SNDMORE)
        self.socket.send(reply)


DEFAULT_IP = "127.0.0.1"


def print_usage():
    print("You should specify the IP address of the device running hammerhead:\n\n"
          "     python hammerhead_scheduler.py IP\n\n"
          "e.g. python hammerhead_scheduler.py 192.168.1.9\n\n"
          "In the meantime, we are going to assume that you are running this on the same device as hammerhead,\n"
          f"that is, we assume that you specified {DEFAULT_IP}"
          "\n----------------------------------------"
          )


def main():
    if len(sys.argv) == 1:
        print_usage()

    ip = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_IP
    endpoint = f"tcp://{ip}:9814"  # Corresponds to WAIT_TOPIC
    scheduler = Scheduler(endpoint)

    # Infinite loop that runs
    try:
        while True:
            scheduler.loop()
    except KeyboardInterrupt:
        print("\nExiting...")


if __name__ == "__main__":
    main()
