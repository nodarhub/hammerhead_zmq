import os
import shutil
import sys

import cv2
import numpy as np
from depth_to_disparity.details import Details
from depth_to_disparity.get_files import get_files
from depth_to_disparity.safe_load import safe_load
from tqdm import tqdm


def main():
    if len(sys.argv) < 2:
        print("Expecting at least one argument (the path to the recorded data). " +
              "Usage:\n\n\tdepth_to_disparity data_directory [output_directory]")
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
    print(f"Found {len(exrs)} depth maps to convert to disparities")

    for exr in tqdm(exrs):
        depth_image = safe_load(exr, cv2.IMREAD_UNCHANGED, [np.float32, ], 1)
        if depth_image is None:
            continue

        details_filename = os.path.join(details_dir, os.path.splitext(os.path.basename(exr))[0] + ".csv")
        if not os.path.exists(details_filename):
            print(f"Could not find the corresponding details for {exr}. This path does not exist: {details_filename}")
            continue
        details = Details(details_filename)

        img_disparity = np.divide((16.0 * details.focal_length * details.baseline), depth_image)
        img_disparity = np.clip(img_disparity, 0, 65535).astype(np.uint16)

        file_path = os.path.join(output_dir, os.path.splitext(os.path.basename(exr))[0] + ".tiff")
        cv2.imwrite(file_path, img_disparity, [cv2.IMWRITE_TIFF_COMPRESSION, 1])


if __name__ == "__main__":
    main()
