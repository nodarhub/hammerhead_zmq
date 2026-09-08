#pragma once

#include <nodar/zmq/image.hpp>
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

inline StampedImage stampedImageFromCvMat(uint64_t time, uint64_t frame_id, uint8_t cvt_to_bgr_code_arg,
                                          const cv::Mat& mat) {
    return StampedImage(time, frame_id, static_cast<uint32_t>(mat.rows), static_cast<uint32_t>(mat.cols),
                        static_cast<uint32_t>(mat.type()), cvt_to_bgr_code_arg, mat.data);
}

inline StampedImage stampedImageFromCvMat(uint64_t time, uint64_t frame_id, const cv::Mat& mat) {
    return stampedImageFromCvMat(time, frame_id, StampedImage::COLOR_CONVERSION::UNSPECIFIED, mat);
}

// Non-owning view of a serialized StampedImage: the header fields plus a cv::Mat pointing into the buffer.
// Only valid while the underlying message buffer is alive.
struct StampedImageView {
    uint64_t time{0};
    uint64_t frame_id{0};
    uint8_t cvt_to_bgr_code{StampedImage::COLOR_CONVERSION::UNSPECIFIED};
    cv::Mat img;
    const uint8_t* additional_field{nullptr};
    uint16_t additional_field_size{0};
};

// Parses a StampedImage message without copying the image payload.
// Returns false if the buffer is not a StampedImage or is smaller than its header claims.
inline bool stampedImageViewFromBuffer(const uint8_t* data, size_t size, StampedImageView& view) {
    if (size < StampedImage::HEADER_SIZE) {
        return false;
    }

    const uint8_t* header = data;

    MessageInfo info;
    header = utils::read(header, info);
    if (info != StampedImage::getInfo()) {
        std::cerr << "This message either is not an image message, or is a different message version." << std::endl;
        return false;
    }

    uint32_t rows;
    uint32_t cols;
    uint32_t type;
    header = utils::read(header, view.time);
    header = utils::read(header, view.frame_id);
    header = utils::read(header, rows);
    header = utils::read(header, cols);
    header = utils::read(header, type);
    header = utils::read(header, view.cvt_to_bgr_code);
    header = utils::read(header, view.additional_field_size);

    const uint64_t image_bytes = StampedImage::dataSize(rows, cols, type, 0);
    if (size < StampedImage::HEADER_SIZE + image_bytes + view.additional_field_size) {
        std::cerr << "According to its header, this image message should be larger than it is. Ignoring it."
                  << std::endl;
        return false;
    }

    uint8_t* payload = const_cast<uint8_t*>(data) + StampedImage::HEADER_SIZE;
    view.img = cv::Mat(static_cast<int>(rows), static_cast<int>(cols), static_cast<int>(type), payload);
    view.additional_field = payload + image_bytes;

    return true;
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
    if (cvt_to_bgr_code == nodar::zmq::StampedImage::COLOR_CONVERSION::BGR2BGR) {
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
    } else if (cvt_to_bgr_code == cv::COLOR_GRAY2BGR) {
        // Greyscale (1 Channel)
        if (!((depth == CV_8U || depth == CV_16U) && channels == 1)) {
            std::cerr << "[ERROR] Invalid greyscale image type.\n"
                      << "  Received: depth=" << depthToString(depth) << ", channels=" << channels << "\n"
                      << "  Expected: depth=CV_8U or CV_16U, channels=1\n";
            return false;
        }
    } else {
        // Cast to int so that the code prints as a number rather than as a character.
        std::cerr << "[ERROR] Unknown cvt_to_bgr_code: " << static_cast<int>(cvt_to_bgr_code) << "\n"
                  << "  Supported cvt_to_bgr_code:\n"
                  << "    - StampedImage::COLOR_CONVERSION::BGR2BGR\n"
                  << "    - cv::COLOR_BayerBG2BGR\n"
                  << "    - cv::COLOR_BayerGB2BGR\n"
                  << "    - cv::COLOR_BayerRG2BGR\n"
                  << "    - cv::COLOR_BayerGR2BGR\n"
                  << "    - cv::COLOR_GRAY2BGR\n";
        return false;
    }

    return true;
}
}  // namespace zmq
}  // namespace nodar
