#pragma once

#include <filesystem>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/publisher.hpp>
#include <opencv2/opencv.hpp>
#include <string>

namespace nodar {
namespace zmq {

class TopbotPublisher {
public:
    explicit TopbotPublisher(const Topic& topic_) : topic(topic_), publisher(topic, "") {}

    bool publishImage(const cv::Mat& img, const uint64_t& timestamp, const uint64_t& frame_id) {
        if (img.empty()) {
            std::cerr << "Failed to load image" << std::endl;
            return false;
        }

        if (!isValidExternalTopic(img, topic)) {
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
