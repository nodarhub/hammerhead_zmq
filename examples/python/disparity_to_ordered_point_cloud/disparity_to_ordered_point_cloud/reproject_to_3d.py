# #REPROJECTION_TO_3D_CODES:6B682632
# This implementation is duplicated across ZMQ and ROS2 examples.
# If you change this file, update ALL files tagged with #REPROJECTION_TO_3D_CODES:6B682632.

import cv2
import numpy as np


def reproject_image_to_3d(
    disparity: np.ndarray,
    projection_type: int,
    Q: np.ndarray,
    rotation_matrix: np.ndarray,
    fy: float = None,
    q_with_reverse_t_vec: bool = True,
    dst: np.ndarray = None,
) -> np.ndarray:
    """Reproject a scaled float32 disparity image into a 3-channel float32 XYZ point cloud,
    rotating the result into world space.

    Args:
        disparity: float32 disparity image, subpixel-scaled
            (divide raw uint16 disparity by 16 before passing in).
        projection_type: 0 = pinhole/rectilinear (only supported value).
            Any other value prints a warning and falls back to pinhole.
        Q: 4x4 disparity-to-depth matrix, float32.
        rotation_matrix: 3x3 rotation from disparity frame to world frame, float32.
            Typically: rotation_world_to_raw_cam.T @ rotation_disparity_to_raw_cam
        fy: optional focal length in y direction in pixels if different from fx.
        q_with_reverse_t_vec: when True (default), negates the last row of the rotated
            Q matrix. This matches the Q-matrix convention used by Hammerhead.
        dst: optional pre-allocated output array of shape (H, W, 3) float32 for reuse.

    Returns:
        3D point cloud image of shape (H, W, 3), float32.
    """
    if projection_type != 0:
        print(
            f"[WARN] reproject_image_to_3d: unsupported projection_type {projection_type} "
            "(expected 0 for pinhole). Continuing with pinhole projection."
        )
    # Embed the 3x3 rotation into a 4x4 identity matrix, then left-multiply Q
    # to rotate the resulting 3D points into world space.
    rotation_4x4 = np.eye(4, dtype=np.float32)
    rotation_4x4[:3, :3] = rotation_matrix.astype(np.float32)
    rotated_Q = rotation_4x4 @ Q
    if q_with_reverse_t_vec:
        rotated_Q[3, :] *= -1
    return cv2.reprojectImageTo3D(disparity, rotated_Q, dst)
