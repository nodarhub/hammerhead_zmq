#pragma once

// #REPROJECTION_TO_3D_CODES:6B682632
// This implementation is duplicated across ZMQ and ROS2 examples.
// If you change this file, update ALL files tagged with #REPROJECTION_TO_3D_CODES:6B682632.

#include <cstdint>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <optional>

namespace nodar {

/// Reproject a scaled float32 disparity image into a 3-channel float32 XYZ point cloud,
/// rotating the result into world space.
///
/// @param point_cloud_image  Output: 3-channel float32 cv::Mat, same size as \p disparity.
///                           May be pre-allocated for reuse across frames.
/// @param projection_type    0 = pinhole/rectilinear (only supported value).
///                           Any other value logs a warning and falls back to pinhole.
/// @param disparity          Input: float32 disparity image, subpixel-scaled
///                           (divide raw uint16 disparity by 16 before passing in).
/// @param Q                  4×4 disparity-to-depth matrix, CV_32FC1.
/// @param rotation_matrix    3×3 rotation from disparity frame to world frame, CV_32FC1.
///                           Typically: rotation_world_to_raw_cam.t() * rotation_disparity_to_raw_cam
/// @param fy                 Optional focal length in y direction in pixels if different from fx.
/// @param q_with_reverse_t_vec  When true (default), negates the last row of the rotated Q matrix.
///                              This matches the Q-matrix convention used by Hammerhead.
inline void reprojectImageTo3D(
    cv::Mat &point_cloud_image,      //
    const int16_t projection_type,   //
    const cv::Mat &disparity,        //
    const cv::Mat &Q,                //
    const cv::Mat &rotation_matrix,  //
    std::optional<float> fy = std::nullopt,  //
    bool q_with_reverse_t_vec = true //
) {
    if (projection_type != 0) {
        std::cerr << "[WARN] nodar::reprojectImageTo3D: unsupported projection_type "
                  << projection_type << " (expected 0 for pinhole). "
                  << "Continuing with pinhole projection." << std::endl;
    }
    // Embed the 3×3 rotation into a 4×4 identity matrix, then left-multiply Q
    // to rotate the resulting 3D points into world space.
    cv::Mat1f rotation_4x4 = cv::Mat::eye(4, 4, CV_32F);
    rotation_matrix.convertTo(rotation_4x4(cv::Rect(0, 0, 3, 3)), CV_32F);
    cv::Mat rotated_Q = rotation_4x4 * Q;
    if (q_with_reverse_t_vec) {
        rotated_Q.row(3) = -rotated_Q.row(3);
    }
    cv::reprojectImageTo3D(disparity, point_cloud_image, rotated_Q);
}

}  // namespace nodar
