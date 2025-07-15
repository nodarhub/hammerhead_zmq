import argparse
import os
import shutil

import cv2
import numpy as np
from details import Details
from tqdm import tqdm


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
                f"Error loading {filename}. The image is supposed to have one of the types {valid_types}, "
                f"not {img.dtype}"
            )
            return None
        num_channels = 1 if len(img.shape) == 2 else img.shape[2]
        if num_channels != expected_num_channels:
            print(
                f"Error loading {filename}. The image is supposed to have {expected_num_channels} channels, "
                f"not {num_channels}"
            )
            return None
        return img
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None


def disparity_to_ordered_pointcloud(disp_s16: np.ndarray, Q: np.ndarray) -> np.ndarray:
    disp_f32 = disp_s16.astype(np.float32) / 16.0
    Q[3, :] = -Q[3, :]
    xyz = cv2.reprojectImageTo3D(disp_f32, Q)
    return xyz


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
        disparity_image = safe_load(
            tiff,
            cv2.IMREAD_UNCHANGED,
            [
                np.uint16,
            ],
            1,
        )
        if disparity_image is None:
            continue

        details_filename = os.path.join(
            details_dir, os.path.splitext(os.path.basename(tiff))[0] + ".yaml"
        )
        if not os.path.exists(details_filename):
            print(
                f"Could not find the corresponding details for {tiff}. This path does not exist: {details_filename}"
            )
            continue
        details = Details(details_filename)

        xyz = disparity_to_ordered_pointcloud(disparity_image, details.disparity_to_depth4x4)

        stem = os.path.splitext(os.path.basename(tiff))[0]

        if split:
            for idx, axis in enumerate("xyz"):
                np.savetxt(os.path.join(output_dir, f"{stem}_x.txt"), xyz[..., 0], fmt="%.6f")
                np.savetxt(os.path.join(output_dir, f"{stem}_y.txt"), xyz[..., 1], fmt="%.6f")
                np.savetxt(os.path.join(output_dir, f"{stem}_z.txt"), xyz[..., 2], fmt="%.6f")
        else:
            fname = os.path.join(output_dir, f"{stem}.tiff")
            cv2.imwrite(fname, xyz, [cv2.IMWRITE_TIFF_COMPRESSION, 1])


if __name__ == "__main__":
    main()
