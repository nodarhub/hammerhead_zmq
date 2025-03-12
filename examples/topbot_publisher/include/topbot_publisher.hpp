#pragma once

#include <filesystem>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/publisher.hpp>
#include <opencv2/opencv.hpp>
#include <string>

namespace nodar {
namespace zmq {

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

inline auto isValidTopic(const cv::Mat& img, const Topic& topic) {
    const auto depth = img.depth();
    const auto channels = img.channels();

    if (topic.name == EXTERNAL_IMAGE_TOPICS[0].name) {
        // BGR Format (3 Channels)
        if (!((depth == CV_8U || depth == CV_16U) && channels == 3)) {
            std::cerr << "[ERROR] Invalid BGR image type.\n"
                      << "  Received: depth=" << depthToString(depth) << ", channels=" << channels << "\n"
                      << "  Expected: depth=CV_8U or CV_16U, channels=3\n";
            return false;
        }
    } else if (topic.name == EXTERNAL_IMAGE_TOPICS[1].name) {
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
                  << "    - " << EXTERNAL_IMAGE_TOPICS[0].name << "\n"
                  << "    - " << EXTERNAL_IMAGE_TOPICS[1].name << "\n";
        return false;
    }

    return true;
}

class TopbotPublisher {
public:
    explicit TopbotPublisher(const Topic& topic_) : topic(topic_), publisher(topic, "") {}

    bool publishImage(const cv::Mat& img, const uint64_t& timestamp, const uint64_t& frame_id) {
        if (img.empty()) {
            std::cerr << "Failed to load image" << std::endl;
            return false;
        }

        if (!isValidTopic(img, topic)) {
            return false;
        }

        auto buffer = publisher.getBuffer();
        buffer->resize(nodar::zmq::StampedImage::msgSize(img.rows, img.cols, img.type()));
        nodar::zmq::StampedImage::write(buffer->data(), timestamp, frame_id, img.rows, img.cols, img.type(), img.data);
        publisher.send(buffer);
        return true;
    }

private:
    Topic topic;
    nodar::zmq::Publisher<nodar::zmq::StampedImage> publisher;
};

}  // namespace zmq
}  // namespace nodar
