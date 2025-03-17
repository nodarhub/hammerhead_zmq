#pragma once

#include <nodar/zmq/image.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

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

inline auto depthToString(const int& depth) {
    switch (depth) {
        case CV_8U:
            return "CV_8U";
        case CV_8S:
            return "CV_8S";
        case CV_16U:
            return "CV_16U";
        case CV_16S:
            return "CV_16S";
        case CV_32S:
            return "CV_32S";
        case CV_32F:
            return "CV_32F";
        case CV_64F:
            return "CV_64F";
        default:
            return "Unknown depth";
    }
}

inline auto isValidExternalImage(const cv::Mat& img, const uint8_t& cvt_to_bgr_code) {
    const auto depth = img.depth();
    const auto channels = img.channels();

    if (cvt_to_bgr_code == 0) {
        // BGR Format (3 Channels)
        if (!((depth == CV_8U || depth == CV_16U) && channels == 3)) {
            std::cerr << "[ERROR] Invalid BGR image type.\n"
                      << "  Received: depth=" << depthToString(depth) << ", channels=" << channels << "\n"
                      << "  Expected: depth=CV_8U or CV_16U, channels=3\n";
            return false;
        }
    } else if (cvt_to_bgr_code == cv::COLOR_BayerBG2BGR or cvt_to_bgr_code == cv::COLOR_BayerGB2BGR or
               cvt_to_bgr_code == cv::COLOR_BayerRG2BGR or cvt_to_bgr_code == cv::COLOR_BayerGR2BGR) {
        // Bayer Format (1 Channel)
        if (!((depth == CV_8U || depth == CV_16U) && channels == 1)) {
            std::cerr << "[ERROR] Invalid Bayer image type.\n"
                      << "  Received: depth=" << depthToString(depth) << ", channels=" << channels << "\n"
                      << "  Expected: depth=CV_8U or CV_16U, channels=1\n";
            return false;
        }
    } else {
        std::cerr << "[ERROR] Unknown cvt_to_bgr_code: " << cvt_to_bgr_code << "\n"
                  << "  Supported cvt_to_bgr_code:\n"
                  << "    - 0\n"
                  << "    - cv::COLOR_BayerBG2BGR\n"
                  << "    - cv::COLOR_BayerGB2BGR\n"
                  << "    - cv::COLOR_BayerRG2BGR\n"
                  << "    - cv::COLOR_BayerGR2BGR\n";
        return false;
    }

    return true;
}
}  // namespace zmq
}  // namespace nodar
