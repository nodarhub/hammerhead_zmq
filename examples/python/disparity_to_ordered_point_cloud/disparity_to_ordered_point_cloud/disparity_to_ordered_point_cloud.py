import argparse
import os
import shutil
import sys

import cv2
import numpy as np
from tqdm import tqdm

try:
    from zmq_msgs.details import Details
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.details import Details


def parse_args():
    p = argparse.ArgumentParser(description="Convert disparity to ordered point cloud")
    p.add_argument("disparity_dir", help="Folder of .tiff disparity images")
    p.add_argument("details_dir", help="Folder of matching .yaml details files")
    p.add_argument("output_dir", help="Destination folder for ordered point clouds")
    p.add_argument(
        "--split", action="store_true", help="Save x, y, z as three separate *.txt files"
    )
    return p.parse_args()


def get_files(dirname, ext):
    files = [os.path.join(dirname, f) for f in os.listdir(dirname) if f.endswith(ext)]
    files.sort()
    return files


def safe_load(filename, read_mode, valid_types, expected_num_channels):
    if not os.path.exists(filename):
        print(f"{filename} does not exist:\n")
        return None
    try:
        img = cv2.imread(filename, read_mode)
        if img is None:
            print(f"Error loading {filename}. The image is empty")
            return None
        if img.dtype not in valid_types:
            print(
                f"Error loading {filename}. "
                f"The image is supposed to have one of the types {valid_types}, "
                f"not {img.dtype}"
            )
            return None
        num_channels = 1 if len(img.shape) == 2 else img.shape[2]
        if num_channels != expected_num_channels:
            print(
                f"Error loading {filename}. "
                f"The image is supposed to have {expected_num_channels} channels, "
                f"not {num_channels}"
            )
            return None
        return img
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None


def write_ascii(filename, arr):
    np.savetxt(filename, arr, fmt="%.6f")


def main():
    args = parse_args()
    disparity_dir = args.disparity_dir
    details_dir = args.details_dir
    output_dir = args.output_dir
    split = args.split

    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)

    # Load the disparity data
    tiffs = get_files(disparity_dir, ".tiff")

    print(f"Found {len(tiffs)} disparity maps to convert to ordered point clouds.")

    for tiff in tqdm(tiffs):
        disparity = safe_load(
            tiff,
            cv2.IMREAD_UNCHANGED,
            [
                np.uint16,
            ],
            1,
        )
        if disparity is None:
            continue

        details_filename = os.path.join(
            details_dir, os.path.splitext(os.path.basename(tiff))[0] + ".yaml"
        )
        if not os.path.exists(details_filename):
            print(
                f"Could not find the corresponding details for {tiff}. "
                f"This path does not exist: {details_filename}"
            )
            continue
        details = Details(details_filename)

        disparity_to_depth4x4 = details.disparity_to_depth4x4
        rotation_disparity_to_raw_cam = details.rotation_disparity_to_raw_cam
        rotation_world_to_raw_cam = details.rotation_world_to_raw_cam

        # Account for subpixel scaling
        disparity_scaled = disparity / np.float32(16)

        # Compute disparity_to_rotated_depth4x4 (rotated Q matrix)
        rotation_disparity_to_world = rotation_world_to_raw_cam.T @ rotation_disparity_to_raw_cam
        rotation_disparity_to_world_4x4 = np.eye(4, dtype=np.float32)
        rotation_disparity_to_world_4x4[:3, :3] = rotation_disparity_to_world
        disparity_to_rotated_depth4x4 = rotation_disparity_to_world_4x4 @ disparity_to_depth4x4

        # Negate the last row of the Q-matrix
        disparity_to_rotated_depth4x4[3, :] *= -1
        xyz = cv2.reprojectImageTo3D(disparity_scaled, disparity_to_rotated_depth4x4)

        stem = os.path.splitext(os.path.basename(tiff))[0]
        if split:
            write_ascii(os.path.join(output_dir, f"{stem}_x.txt"), xyz[..., 0])
            write_ascii(os.path.join(output_dir, f"{stem}_y.txt"), xyz[..., 1])
            write_ascii(os.path.join(output_dir, f"{stem}_z.txt"), xyz[..., 2])
        else:
            cv2.imwrite(
                os.path.join(output_dir, f"{stem}.tiff"), xyz, [cv2.IMWRITE_TIFF_COMPRESSION, 1]
            )


if __name__ == "__main__":
    main()
