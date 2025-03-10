import os

import cv2


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
            print(f"Error loading {filename}. The image is supposed to have one of the types {valid_types}, "
                  f"not {img.dtype}")
        num_channels = 1 if len(img.shape) == 2 else img.shape[2]
        if num_channels != expected_num_channels:
            print(f"Error loading {filename}. The image is supposed to have {expected_num_channels} channels, "
                  f"not {num_channels}")
            return None
        return img
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None
