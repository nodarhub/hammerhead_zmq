#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/publisher.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

#include "get_files.hpp"

constexpr auto FRAME_RATE = 10;
std::atomic_bool running{true};

void signalHandler(int) {
    std::cerr << "SIGINT or SIGTERM received. Exiting..." << std::endl;
    running = false;
}

std::string depthToString(const int& depth) {
    switch (depth) {
        case CV_8U:
            return "CV_8U (8-bit unsigned)";
        case CV_8S:
            return "CV_8S (8-bit signed)";
        case CV_16U:
            return "CV_16U (16-bit unsigned)";
        case CV_16S:
            return "CV_16S (16-bit signed)";
        case CV_32S:
            return "CV_32S (32-bit signed)";
        case CV_32F:
            return "CV_32F (32-bit float)";
        case CV_64F:
            return "CV_64F (64-bit float)";
        default:
            return "Unknown depth";
    }
}

class TopbotPublisher {
public:
    TopbotPublisher() : publisher(nodar::zmq::IMAGE_TOPICS[6], "") {}

    bool publishImage(const cv::Mat& img, const uint64_t& timestamp, const uint64_t& frame_id) {
        if (img.empty()) {
            std::cerr << "Failed to load image" << std::endl;
            return false;
        }
        const auto depth = img.depth();
        const auto channels = img.channels();

        if (!((depth == CV_8U || depth == CV_16U) && channels == 3)) {
            std::cerr << "Skipping unsupported image type: depth=" << depthToString(depth) << ", channels=" << channels
                      << std::endl;
            return false;
        }

        auto buffer = publisher.getBuffer();
        buffer->resize(nodar::zmq::StampedImage::msgSize(img.rows, img.cols, img.type()));
        nodar::zmq::StampedImage::write(buffer->data(), timestamp, frame_id, img.rows, img.cols, img.type(), img.data);
        publisher.send(buffer);
        return true;
    }

private:
    nodar::zmq::Publisher<nodar::zmq::StampedImage> publisher;
};

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (argc < 2) {
        std::cerr << "Usage: topbot_publisher topbot_data_directory" << std::endl;
        return EXIT_FAILURE;
    }

    const auto image_files{getFiles(argv[1], ".tiff")};
    if (image_files.empty()) {
        std::cerr << "No images found in folder." << std::endl;
        return EXIT_FAILURE;
    }

    TopbotPublisher publisher;
    auto frame_id = 0;

    while (running) {
        for (const auto& file : image_files) {
            if (!running)
                break;
            const auto img = cv::imread(file, cv::IMREAD_UNCHANGED);
            const auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();
            if (publisher.publishImage(img, timestamp, frame_id)) {
                std::cout << "Published frame " << frame_id << " from " << file << std::endl;
                frame_id++;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FRAME_RATE));
        }
    }

    std::cout << "Publisher stopped." << std::endl;
    return EXIT_SUCCESS;
}
