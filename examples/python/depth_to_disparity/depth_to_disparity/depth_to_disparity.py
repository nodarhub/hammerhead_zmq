import os
import shutil
import sys

import cv2
import numpy as np
import yaml
from tqdm import tqdm

os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"


def read_scalar(config: dict, key: str, expected_type: type):
    if key not in config:
        raise KeyError(f"Missing required key: '{key}'")

    value = config[key]

    # Type coercion and validation
    try:
        if expected_type is float:
            return float(value)
        elif expected_type is int:
            return int(value)
        else:
            raise TypeError(f"Unsupported expected_type: {expected_type}")
    except (ValueError, TypeError) as e:
        raise ValueError(f"Field '{key}' must be a {expected_type.__name__}: {e}")


class Details:
    def __init__(self, filename):
        self.disparity_to_depth4x4 = np.zeros((4, 4), dtype=np.float32)
        with open(filename, "r") as details_file:
            config = yaml.safe_load(details_file)

            self.focal_length = read_scalar(config, "focal_length", float)
            self.baseline = read_scalar(config, "baseline", float)

    def __str__(self):
        return (
            "Details:\n"
            + f"\tfocal_length                  : {self.focal_length}\n"
            + f"\tbaseline                      : {self.baseline}\n"
        )


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
                f"The image is supposed to have one of the types {valid_types}, not {img.dtype}"
            )
            return None
        num_channels = 1 if len(img.shape) == 2 else img.shape[2]
        if num_channels != expected_num_channels:
            print(
                f"Error loading {filename}. The image is supposed to have "
                f"{expected_num_channels} channels, not {num_channels}"
            )
            return None
        return img
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None


def main():
    if len(sys.argv) < 2:
        print(
            "Expecting at least one argument (the path to the recorded data). "
            + "Usage:\n\n\tdepth_to_disparity data_directory [output_directory]"
        )
        return

    input_dir = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else os.path.join(input_dir, "disparity")

    # Directories that we read
    depth_dir = os.path.join(input_dir, "depth")
    details_dir = os.path.join(input_dir, "details")

    # Remove old output directory if it exists
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)

    # Load the depth data
    exrs = get_files(depth_dir, ".exr")
    tiffs = get_files(depth_dir, ".tiff")

    if len(exrs) > 0 and len(tiffs) == 0:
        print("Found depth maps in EXR format. Converting to TIFF format.")
        for exr in tqdm(exrs):
            depth_image = safe_load(
                exr,
                cv2.IMREAD_UNCHANGED,
                [
                    np.float32,
                ],
                1,
            )
            if depth_image is None:
                continue
            file_path = os.path.join(
                depth_dir, os.path.splitext(os.path.basename(exr))[0] + ".tiff"
            )
            cv2.imwrite(file_path, depth_image, [cv2.IMWRITE_TIFF_COMPRESSION, 1])
        tiffs = get_files(depth_dir, ".tiff")

    print(f"Found {len(tiffs)} depth maps to convert to disparities")

    for tiff in tqdm(tiffs):
        depth_image = safe_load(
            tiff,
            cv2.IMREAD_UNCHANGED,
            [
                np.float32,
            ],
            1,
        )
        if depth_image is None:
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

        img_disparity = np.divide((16.0 * details.focal_length * details.baseline), depth_image)
        img_disparity = np.clip(img_disparity, 0, 65535).astype(np.uint16)

        file_path = os.path.join(output_dir, os.path.splitext(os.path.basename(tiff))[0] + ".tiff")
        cv2.imwrite(file_path, img_disparity, [cv2.IMWRITE_TIFF_COMPRESSION, 1])


if __name__ == "__main__":
    main()
