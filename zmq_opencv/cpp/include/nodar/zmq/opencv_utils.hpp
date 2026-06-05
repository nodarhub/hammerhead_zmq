#pragma once

#include <nodar/zmq/image.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace nodar {
namespace zmq {
inline cv::Mat cvMatFromStampedImage(const StampedImage& stamped_image) {
    if (stamped_image.img.empty()) {
        return cv::Mat();
    }
    // Wrap the payload with its true row stride (which may be padded, e.g. a GpuMat on Tegra),
    // then clone so the returned Mat owns a continuous, padding-free copy.
    const cv::Mat view(static_cast<int>(stamped_image.rows), static_cast<int>(stamped_image.cols),
                       static_cast<int>(stamped_image.type), const_cast<uint8_t*>(stamped_image.img.data()),
                       static_cast<size_t>(stamped_image.step));
    return view.clone();
}

inline StampedImage stampedImageFromCvMat(uint64_t time, uint64_t frame_id, uint8_t cvt_to_bgr_code_arg,
                                          const cv::Mat& mat) {
    if (mat.isContinuous()) {
        // Tightly packed: the field constructor records the packed stride for us.
        return StampedImage(time, frame_id, static_cast<uint32_t>(mat.rows), static_cast<uint32_t>(mat.cols),
                            static_cast<uint32_t>(mat.type()), cvt_to_bgr_code_arg, mat.data);
    }

    // Non-continuous Mat (e.g. an ROI): pack it into our own buffer. We size the buffer for the packed
    // image, wrap it in a Mat header, and let OpenCV copy into it -- one copy, no intermediary clone. We
    // store the packed stride, not mat.step (the parent's stride, which can be far larger than we need).
    StampedImage out;
    out.time = time;
    out.frame_id = frame_id;
    out.rows = static_cast<uint32_t>(mat.rows);
    out.cols = static_cast<uint32_t>(mat.cols);
    out.type = static_cast<uint32_t>(mat.type());
    out.cvt_to_bgr_code = cvt_to_bgr_code_arg;
    out.step = StampedImage::packedStep(out.cols, out.type);
    out.img.resize(out.dataSize());
    cv::Mat dst(static_cast<int>(out.rows), static_cast<int>(out.cols), static_cast<int>(out.type), out.img.data());
    mat.copyTo(dst);
    return out;
}

inline StampedImage stampedImageFromCvMat(uint64_t time, uint64_t frame_id, const cv::Mat& mat) {
    return stampedImageFromCvMat(time, frame_id, StampedImage::COLOR_CONVERSION::UNSPECIFIED, mat);
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
    } else {
        std::cerr << "[ERROR] Unknown cvt_to_bgr_code: " << cvt_to_bgr_code << "\n"
                  << "  Supported cvt_to_bgr_code:\n"
                  << "    - StampedImage::COLOR_CONVERSION::BGR2BGR\n"
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
