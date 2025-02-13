#include <chrono>
#include <filesystem>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/publisher.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>

constexpr auto FRAME_RATE = 10;

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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: topbot_publisher <topbot_image_folder>" << std::endl;
        return EXIT_FAILURE;
    }

    const std::filesystem::path folder{argv[1]};
    auto image_files = getImageFiles(folder);
    if (image_files.empty()) {
        std::cerr << "No images found in folder: " << folder << std::endl;
        return EXIT_FAILURE;
    }

    nodar::zmq::Publisher<nodar::zmq::StampedImage> publisher(nodar::zmq::IMAGE_TOPICS[6], "");
    uint64_t frame_id = 0;

    while (true) {
        for (const auto& file : image_files) {
            cv::Mat img = cv::imread(file, cv::IMREAD_COLOR);
            if (img.empty()) {
                std::cerr << "Failed to load image: " << file << std::endl;
                continue;
            }

            uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();

            auto buffer = publisher.getBuffer();
            buffer->resize(nodar::zmq::StampedImage::msgSize(img.rows, img.cols, img.type()));
            nodar::zmq::StampedImage::write(buffer->data(), timestamp, frame_id, img.rows, img.cols, img.type(),
                                            img.data);
            publisher.send(buffer);

            std::cout << "Published frame " << frame_id << " from " << file << std::endl;
            frame_id++;

            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FRAME_RATE));
        }
    }
}
