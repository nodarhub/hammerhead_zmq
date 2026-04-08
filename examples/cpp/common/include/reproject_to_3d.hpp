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
/// @param projection_type    0 = pinhole/rectilinear (only supported value), 1 = cylindrical-Y.
///                           Any other value logs a warning and performs no operation.
/// @param disparity          Input: float32 disparity image, subpixel-scaled
///                           (divide raw uint16 disparity by 16 before passing in).
/// @param Q                  4×4 disparity-to-depth matrix, CV_32FC1.
/// @param rotation_matrix    3×3 rotation from disparity frame to world frame, CV_32FC1.
///                           Typically: rotation_world_to_raw_cam.t() * rotation_disparity_to_raw_cam
/// @param fy                 Optional focal length in y direction in pixels if different from fx.
/// @param q_with_reverse_t_vec  When true (default), negates the last row of the rotated Q matrix.
///                              This matches the Q-matrix convention used by Hammerhead.
inline void reprojectImageTo3D(cv::Mat &point_cloud_image,  //
                               const int16_t projection_type,  //
                               const cv::Mat &disparity,  //
                               const cv::Mat &Q,  //
                               const cv::Mat &rotation_matrix,  //
                               std::optional<float> fy = std::nullopt,  //
                               bool q_with_reverse_t_vec = true) {
    if (projection_type == 0) {
        cv::Mat Q_local = Q;
        if (fy.has_value()) {
            // Change for aspect ratio correction. 
            // This has to be done before the rotation because it affects the pixel coordinates.
            Q_local = Q.clone();
            const float fx = Q.at<float>(2, 3);
            const float aspect_multiplier = fx / fy.value();  // fy in denominator to produce (v - cy) / fy.
            Q_local.at<float>(1, 1) *= aspect_multiplier;
            Q_local.at<float>(1, 3) *= aspect_multiplier;
        }
        // Embed the 3×3 rotation into a 4×4 identity matrix, then left-multiply Q
        // to rotate the resulting 3D points into world space.
        cv::Mat1f rotation_4x4 = cv::Mat::eye(4, 4, CV_32F);
        if (!rotation_matrix.empty()) {
            rotation_matrix.convertTo(rotation_4x4(cv::Rect(0, 0, 3, 3)), CV_32F);
        }
        cv::Mat1f rotated_Q = rotation_4x4 * Q_local;
        if (q_with_reverse_t_vec) {
            // Negate the last row of the Q-matrix
            rotated_Q.row(3) = -rotated_Q.row(3);
        }
        cv::reprojectImageTo3D(disparity, point_cloud_image, rotated_Q);
    } else if (projection_type == 1) {
        const float cx1 = -Q.at<float>(0, 3);
        const float cy = -Q.at<float>(1, 3);
        const float fx = Q.at<float>(2, 3);
        const float fy_ = fy.value_or(fx);
        const float baseline = (q_with_reverse_t_vec ? -1.f : 1.f) / Q.at<float>(3, 2);
        // const float cx2 = Q.at<float>(3, 3) / Q.at<float>(3, 2) + cx1;
        const float cx2_minus_cx1 = Q.at<float>(3, 3) / Q.at<float>(3, 2);

        point_cloud_image.create(disparity.size(), CV_32FC3);

        for (int row = 0; row < disparity.rows; ++row) {
            const float angle = (static_cast<float>(row) - cy) / fy_;
            const float sine = std::sin(angle);
            const float cosine = std::cos(angle);

            for (int col = 0; col < disparity.cols; ++col) {
                const float disp = disparity.at<float>(row, col);
                auto &p = point_cloud_image.at<cv::Vec3f>(row, col);
                if (disp <= 0.f) {
                    p = {std::numeric_limits<float>::infinity(),
                         std::numeric_limits<float>::infinity(),
                         std::numeric_limits<float>::infinity()};
                    continue;
                }
                const float cyl_radius = fx * baseline / (disp + cx2_minus_cx1);
                p[0] = cyl_radius * (static_cast<float>(col) - cx1) / fx;
                p[1] = cyl_radius * sine;
                p[2] = cyl_radius * cosine;
            }
        }

        if (!rotation_matrix.empty()) {
            cv::Mat rotation_32f;
            rotation_matrix.convertTo(rotation_32f, CV_32F);
            cv::transform(point_cloud_image, point_cloud_image, rotation_32f);
        }
    } else {
        std::cerr << "Warning: reprojectImageTo3D() -- unsupported projection type " << projection_type
                  << ". Performs no operation." << std::endl;
    }
}

}  // namespace nodar
