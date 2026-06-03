import math
import os
import struct
import sys

import cv2
import numpy as np
import zmq

try:
    from zmq_msgs.image import StampedImage
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.image import StampedImage


def get_image_type(dtype, channels):
    """Get OpenCV type as string"""
    depth_map = {
        np.uint8: "CV_8U",
        np.int8: "CV_8S",
        np.uint16: "CV_16U",
        np.int16: "CV_16S",
        np.int32: "CV_32S",
        np.float32: "CV_32F",
        np.float64: "CV_64F"
    }
    depth_str = depth_map.get(dtype.type, "Unknown")
    return f"{depth_str}C{channels}"


class OccupancyMapMetadata:
    def __init__(self, x_min, x_max, z_min, z_max, cell_size):
        self.x_min = x_min
        self.x_max = x_max
        self.z_min = z_min
        self.z_max = z_max
        self.cell_size = cell_size

    def print(self):
        print("==================================== Occupancy Map Metadata ====================================")
        print(f"X range: [{self.x_min}, {self.x_max}] meters")
        print(f"Z range: [{self.z_min}, {self.z_max}] meters")
        print(f"Cell size: {self.cell_size} meters")
        grid_x = (self.x_max - self.x_min) / self.cell_size
        grid_z = (self.z_max - self.z_min) / self.cell_size
        print(f"Grid dimensions (X x Z): {grid_x:.0f} x {grid_z:.0f} cells")
        print("================================================================================================")


def parse_metadata(additional_field):
    """Parse metadata from additional_field (5 floats = 20 bytes)"""
    expected_size = 5 * 4  # 5 floats, 4 bytes each
    if len(additional_field) != expected_size:
        print(f"Warning: Expected {expected_size} bytes of metadata, got {len(additional_field)} bytes")
        return None

    # Unpack 5 floats: xMin, xMax, zMin, zMax, cellSize
    values = struct.unpack('fffff', additional_field)
    return OccupancyMapMetadata(
        x_min=values[0],
        x_max=values[1],
        z_min=values[2],
        z_max=values[3],
        cell_size=values[4]
    )


def draw_grid_overlay(occupancy_map, metadata):
    """Draw grid overlay with metric labels on the occupancy map"""
    height, width = occupancy_map.shape

    # Margins for labels
    left_margin = 50
    bottom_margin = 50
    top_margin = 20
    right_margin = 20

    # Create larger image with margins
    total_height = top_margin + height + bottom_margin
    total_width = left_margin + width + right_margin

    # Convert to color and add borders
    color_map = cv2.cvtColor(occupancy_map, cv2.COLOR_GRAY2BGR)
    display_img = cv2.copyMakeBorder(color_map, top_margin, bottom_margin, left_margin, right_margin,
                                     cv2.BORDER_CONSTANT, value=(255, 255, 255))  # White margins

    grid_color = (80, 80, 80)  # Gray for grid lines
    x_text_color = (0, 0, 255)  # Red for X axis labels
    z_text_color = (0, 255, 0)  # Green for Z axis labels
    font_scale = 0.3
    font_thickness = 1
    line_thickness = 1

    # Calculate pixels per meter
    # X maps to height (rows), Z maps to width (columns)
    pixels_per_meter_x = height / (metadata.x_max - metadata.x_min)
    pixels_per_meter_z = width / (metadata.z_max - metadata.z_min)

    # Fixed grid spacing of 10 meters
    grid_spacing = 10.0

    # Draw horizontal grid lines for X values
    x_start = math.floor(metadata.x_min / grid_spacing) * grid_spacing
    x = x_start
    while x <= metadata.x_max:
        pixel_x = int((x - metadata.x_min) * pixels_per_meter_x) + top_margin
        if top_margin <= pixel_x <= top_margin + height:
            # Draw grid line across the image area
            cv2.line(display_img, (left_margin, pixel_x), (left_margin + width, pixel_x),
                    grid_color, line_thickness)

            # Draw label in left margin
            label = f"{int(x)}m"
            cv2.putText(display_img, label, (5, pixel_x + 5),
                       cv2.FONT_HERSHEY_SIMPLEX, font_scale, x_text_color, font_thickness)
        x += grid_spacing

    # Draw vertical grid lines for Z values
    z_start = math.floor(metadata.z_min / grid_spacing) * grid_spacing
    z = z_start
    while z <= metadata.z_max:
        pixel_z = int((z - metadata.z_min) * pixels_per_meter_z) + left_margin
        if left_margin <= pixel_z <= left_margin + width:
            # Draw grid line across the image area
            cv2.line(display_img, (pixel_z, top_margin), (pixel_z, top_margin + height),
                    grid_color, line_thickness)

            # Draw label in bottom margin
            label = f"{int(z)}m"
            cv2.putText(display_img, label, (pixel_z - 10, total_height - 8),
                       cv2.FONT_HERSHEY_SIMPLEX, font_scale, z_text_color, font_thickness)
        z += grid_spacing

    return display_img


class OccupancyMapViewer:
    def __init__(self, endpoint):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt(zmq.RCVHWM, 1)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint)
        self.last_frame_id = 0
        self.window_name = "Occupancy Map"
        print(f"Subscribing to {endpoint}")
        cv2.namedWindow(self.window_name, cv2.WINDOW_NORMAL)

    def loop_once(self):
        msg = self.socket.recv()
        stamped_image = StampedImage()
        stamped_image.read(msg)

        img = stamped_image.img
        if img is None or img.size == 0:
            print("Empty image received")
            return True

        frame_id = stamped_image.frame_id
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            print(
                f"{frame_id - self.last_frame_id - 1} frames dropped. "
                f"Current frame ID: {frame_id}, last frame ID: {self.last_frame_id}"
            )
        self.last_frame_id = frame_id

        # Parse metadata from additional_field
        metadata = None
        if stamped_image.additional_field:
            metadata = parse_metadata(stamped_image.additional_field)

        # If image is 3D with 1 channel, squeeze it to 2D
        img = img.squeeze(axis=2)

        # Get image type dynamically
        channels = 1 if len(img.shape) == 2 else img.shape[2]
        img_type = get_image_type(img.dtype, channels)

        occupied_cells = cv2.countNonZero(img)

        print(
            f"Frame # {frame_id} | Time: {stamped_image.time} | "
            f"Size: {img.shape[0]}x{img.shape[1]} | "
            f"Type: {img_type} | Occupied cells: {occupied_cells}"
        )

        if metadata:
            metadata.print()

        # Create visualization with grid overlay
        display_img = draw_grid_overlay(img, metadata)

        # Display the image
        cv2.resizeWindow(self.window_name, 1920, 1080)
        cv2.imshow(self.window_name, display_img)
        cv2.waitKey(1)

        return True


def print_usage(default_ip):
    print(
        "You should specify the IP address of the device running Hammerhead:\n\n"
        "     python occupancy_map_viewer.py hammerhead_ip\n\n"
        "e.g. python occupancy_map_viewer.py 192.168.1.9\n\n"
        "If unspecified, we assume you are running this on the device running Hammerhead,\n"
        f"     python occupancy_map_viewer.py {default_ip}\n\n"
        "Note: Make sure 'enable_grid_detect = 1' in master_config.ini\n"
        "----------------------------------------"
    )


def main():
    default_ip = "127.0.0.1"
    occupancy_map_port = 9900  # From private_topic_ports.hpp

    if len(sys.argv) < 2:
        print_usage(default_ip)

    ip = sys.argv[1] if len(sys.argv) > 1 else default_ip
    endpoint = f"tcp://{ip}:{occupancy_map_port}"

    viewer = OccupancyMapViewer(endpoint)
    try:
        while True:
            viewer.loop_once()
    except KeyboardInterrupt:
        print("\nExiting...")
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()