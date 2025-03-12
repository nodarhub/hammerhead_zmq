#pragma once

#include <nodar/zmq/image.hpp>
#include <nodar/zmq/topic_ports.hpp>
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

inline auto isValidExternalTopic(const cv::Mat& img, const Topic& topic) {
    const auto depth = img.depth();
    const auto channels = img.channels();

    if (topic.name == EXTERNAL_TOPBOT_TOPICS[0].name) {
        // BGR Format (3 Channels)
        if (!((depth == CV_8U || depth == CV_16U) && channels == 3)) {
            std::cerr << "[ERROR] Invalid BGR image type.\n"
                      << "  Received: depth=" << depthToString(depth) << ", channels=" << channels << "\n"
                      << "  Expected: depth=CV_8U or CV_16U, channels=3\n";
            return false;
        }
    } else if (topic.name == EXTERNAL_TOPBOT_TOPICS[1].name) {
        // Bayer BGGR Format (1 Channel)
        if (!((depth == CV_8U || depth == CV_16U) && channels == 1)) {
            std::cerr << "[ERROR] Invalid Bayer BGGR image type.\n"
                      << "  Received: depth=" << depthToString(depth) << ", channels=" << channels << "\n"
                      << "  Expected: depth=CV_8U or CV_16U, channels=1\n";
            return false;
        }
    } else {
        std::cerr << "[ERROR] Unknown topic: " << topic.name << "\n"
                  << "  Supported topics:\n"
                  << "    - " << EXTERNAL_TOPBOT_TOPICS[0].name << "\n"
                  << "    - " << EXTERNAL_TOPBOT_TOPICS[1].name << "\n";
        return false;
    }

    return true;
}
}  // namespace zmq
}  // namespace nodar
