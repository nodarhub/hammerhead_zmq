"""Visualize greyscale or RGB TIFF images by normalizing each channel independently.

This script loads a TIFF image with 3 channels (possibly floats),
scales each channel separately to the full 0-255 range for proper display,
converts the result to 8-bit unsigned integers,
and presents the image using OpenCV's display window.

Usage:
    python visualize_point_cloud_tiff.py <filename>

where <filename> is the path to the input TIFF image file.
"""
import sys

import cv2
import numpy as np


def percentile_clamp_scale(channel):
    valid_mask = np.isfinite(channel)
    if not np.any(valid_mask):
        return np.zeros_like(channel, dtype=np.uint8)
    valid_vals = channel[valid_mask]
    low, high = np.percentile(valid_vals, [10, 90])
    clamped = np.clip(channel, low, high)
    if low == high:
        scaled = np.zeros_like(channel, dtype=channel.dtype)
    else:
        # Clamp to [0, 1]
        scaled = (clamped - low) / (high - low)

        # Center at 0, stretch out the distribution, and then rescale.
        scaled = 2 * scaled - 1
        scaled = scaled**2
        scaled = (scaled + 1) / 2

        # Now convert to uint8 for viewing and 0 the invalid values
        scaled = (scaled * 255).astype(np.uint8)
        scaled[~valid_mask] = 0
    return scaled


def main():
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <filenames>")
        return

    filenames = sys.argv[1:]
    imgs = []
    for filename in filenames:
        img = cv2.imread(filename, cv2.IMREAD_UNCHANGED)
        if img is None:
            print(f"Failed to load image: {filename}")
            return

        # Handle both single-channel and 3-channel images
        if len(img.shape) == 2:
            img_norm = percentile_clamp_scale(img)
        elif len(img.shape) == 3 and img.shape[2] == 3:
            channels = cv2.split(img)
            norm_chans = [percentile_clamp_scale(ch) for ch in channels]
            img_norm = cv2.merge(norm_chans)
        else:
            print("Unsupported image format: expect 1 or 3 channels")
            continue
        imgs.append(img_norm)

    merged_img = np.vstack(imgs)
    window_name = "Normalized TIFF(s)"
    scale = 1 / len(imgs)
    cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
    cv2.imshow(
        window_name,
        cv2.resize(merged_img, (0, 0), fx=scale, fy=scale),
    )
    cv2.waitKey(0)
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
