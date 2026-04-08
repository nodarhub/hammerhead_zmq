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
        projection_type: 0 = pinhole/rectilinear, 1 = cylindrical-Y.
            Any other value prints a warning and performs no operation.
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
    if dst is not None and (dst.shape != disparity.shape + (3,) or dst.dtype != np.float32):
        raise ValueError("dst array has incorrect shape or dtype")

    if projection_type == 0:
        Q_local = Q
        if fy is not None:
            # Aspect ratio correction — must be done before rotation since it affects pixel coords.
            Q_local = Q.copy()
            fx = Q[2, 3]
            aspect_multiplier = fx / fy  # fy in denominator to produce (v - cy) / fy
            Q_local[1, 1] *= aspect_multiplier
            Q_local[1, 3] *= aspect_multiplier
        # Embed the 3x3 rotation into a 4x4 identity matrix, then left-multiply Q
        # to rotate the resulting 3D points into world space.
        rotation_4x4 = np.eye(4, dtype=np.float32)
        rotation_4x4[:3, :3] = rotation_matrix.astype(np.float32)
        rotated_Q = rotation_4x4 @ Q_local
        if q_with_reverse_t_vec:
            rotated_Q[3, :] *= -1
        return cv2.reprojectImageTo3D(disparity, rotated_Q, dst)
    elif projection_type == 1:
        cx1 = -Q[0, 3]
        cy = -Q[1, 3]
        fx = Q[2, 3]
        fy = fy if fy is not None else fx
        baseline = (-1.0 if q_with_reverse_t_vec else 1.0) / Q[3, 2]
        # cx2 = Q[3,3]/Q[3,2] + cx1
        cx2_minus_cx1 = Q[3, 3] / Q[3, 2]

        H, W = disparity.shape
        rows = np.arange(H, dtype=np.float32)
        cols = np.arange(W, dtype=np.float32)
        angle = (rows - cy) / fy
        sine = np.sin(angle)    # shape (H,)
        cosine = np.cos(angle)  # shape (H,)

        disp = disparity.astype(np.float32)
        valid = disp > 0.0

        cyl_radius = np.full((H, W), np.inf, dtype=np.float32)
        cyl_radius[valid] = fx * baseline / (disp[valid] + cx2_minus_cx1)

        if dst is not None:
            point_cloud = dst
        else:
            point_cloud = np.empty((H, W, 3), dtype=np.float32)
        point_cloud[..., 0] = cyl_radius * (cols[np.newaxis, :] - cx1) / fx
        point_cloud[..., 1] = cyl_radius * sine[:, np.newaxis]
        point_cloud[..., 2] = cyl_radius * cosine[:, np.newaxis]
        # Ensure invalid pixels are inf (avoids inf*0=nan edge cases)
        point_cloud[~valid] = np.inf

        if rotation_matrix is not None and rotation_matrix.size > 0:
            # in-place operation is needed for `dst`.
            point_cloud[:] = point_cloud @ rotation_matrix.astype(np.float32).T  # .T for applying on row vectors.

        return point_cloud
    else:
        print(
            f"[WARN] reproject_image_to_3d: unsupported projection_type {projection_type}. "
            "Performing no operation."
        )
        return None
