#pragma once

#include <nodar/zmq/image.hpp>
#include <opencv2/core.hpp>

namespace nodar {
namespace zmq {
inline cv::Mat cvMatFromStampedImage(const StampedImage& stamped_image) {
    cv::Mat mat(static_cast<int>(stamped_image.rows), static_cast<int>(stamped_image.cols),
                static_cast<int>(stamped_image.type));
    memcpy(mat.data, stamped_image.img.data(), stamped_image.img.size());
    return mat;
}

inline StampedImage stampedImageFromCvMat(uint64_t time, uint64_t frame_id, const cv::Mat& mat) {
    return StampedImage(time, frame_id, static_cast<uint32_t>(mat.rows), static_cast<uint32_t>(mat.cols),
                        static_cast<uint32_t>(mat.type()), mat.data);
}
}  // namespace zmq
}  // namespace nodar
