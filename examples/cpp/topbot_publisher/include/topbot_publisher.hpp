#pragma once

#include <array>
#include <filesystem>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/publisher.hpp>
#include <opencv2/opencv.hpp>
#include <optional>
#include <string>

namespace nodar {
namespace zmq {

std::vector<uint8_t> makeExtrinsicsMessage(const std::array<double, 6>& extrinsics) {
    std::array<uint8_t, 16> message_identifier{
        0x2c, 0x5e, 0x9c, 0x77, 0xa7, 0x30, 0x42, 0xce, 0xac, 0x21, 0xc3, 0x3e, 0x26, 0x79, 0x3b, 0xcb};

    auto ret = std::vector<uint8_t>(sizeof(message_identifier) + sizeof(extrinsics));
    auto data = ret.data();

    std::memcpy(data, message_identifier.data(), sizeof(message_identifier));
    std::memcpy(data + sizeof(message_identifier), extrinsics.data(), sizeof(extrinsics));
    return std::move(ret);
}

class TopbotPublisher {
public:
    explicit TopbotPublisher(const uint16_t& port) : publisher(Topic{"external/topbot_raw", port}, "") {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    bool publishImage(const cv::Mat& img, const uint64_t& timestamp, const uint64_t& frame_id,
                      const uint8_t cvt_to_bgr_code, std::optional<std::array<double, 6>> extrinsics = std::nullopt) {
        if (img.empty()) {
            std::cerr << "Failed to load image" << std::endl;
            return false;
        }

        if (!isValidExternalImage(img, cvt_to_bgr_code)) {
            return false;
        }

        if (!img.isContinuous()) {
            // Check if the image is continuous
            std::cerr << "The image is not continuous in memory layout. Currently support continuous images only." << std::endl;
            return false;
        }

        auto additional_field =
            extrinsics.has_value() ? makeExtrinsicsMessage(extrinsics.value()) : std::vector<uint8_t>{};

        auto buffer = publisher.getBuffer();
        buffer->resize(nodar::zmq::StampedImage::msgSize(img.rows, img.cols, img.type(), additional_field.size()));
        nodar::zmq::StampedImage::write(buffer->data(), timestamp, frame_id, img.rows, img.cols, img.type(),
                                        cvt_to_bgr_code, img.data, additional_field.size(), additional_field.data());
        publisher.send(buffer);
        return true;
    }

private:
    nodar::zmq::Publisher<nodar::zmq::StampedImage> publisher;
};

}  // namespace zmq
}  // namespace nodar
