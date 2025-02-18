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
#include <vector>

constexpr auto FRAME_RATE = 10;
std::atomic_bool running{true};

void signalHandler(int) {
    std::cerr << "SIGINT or SIGTERM received. Exiting..." << std::endl;
    running = false;
}

std::vector<std::string> getImageFiles(const std::string& folder) {
    std::vector<std::string> image_files;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".tiff" || entry.path().extension() == ".png")) {
            image_files.push_back(entry.path().string());
        }
    }
    std::sort(image_files.begin(), image_files.end());
    return image_files;
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

    void publishImage(const cv::Mat& img, const uint64_t& timestamp, const uint64_t& frame_id) {
        const auto depth = img.depth();
        const auto channels = img.channels();

        if (!((depth == CV_8U || depth == CV_16U) && channels == 3)) {
            std::cerr << "Skipping unsupported image type: depth=" << depthToString(depth) << ", channels=" << channels
                      << std::endl;
            return;
        }

        auto buffer = publisher.getBuffer();
        buffer->resize(nodar::zmq::StampedImage::msgSize(img.rows, img.cols, img.type()));
        nodar::zmq::StampedImage::write(buffer->data(), timestamp, frame_id, img.rows, img.cols, img.type(), img.data);
        publisher.send(buffer);
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

    auto image_files = getImageFiles(argv[1]);
    if (image_files.empty()) {
        std::cerr << "No images found in folder." << std::endl;
        return EXIT_FAILURE;
    }

    TopbotPublisher publisher;
    uint64_t frame_id = 0;

    while (running) {
        for (const auto& file : image_files) {
            if (!running)
                break;

            cv::Mat img = cv::imread(file, cv::IMREAD_UNCHANGED);
            if (img.empty()) {
                std::cerr << "Failed to load image: " << file << std::endl;
                continue;
            }

            uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();

            publisher.publishImage(img, timestamp, frame_id);
            std::cout << "Published frame " << frame_id << " from " << file << std::endl;
            frame_id++;

            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FRAME_RATE));
        }
    }

    std::cout << "Publisher stopped." << std::endl;
    return EXIT_SUCCESS;
}
