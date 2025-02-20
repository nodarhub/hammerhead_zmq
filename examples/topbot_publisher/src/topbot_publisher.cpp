#include "topbot_publisher.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

#include "get_files.hpp"

constexpr auto FRAME_RATE = 10;
std::atomic_bool running{true};

void signalHandler(int) {
    std::cerr << "SIGINT or SIGTERM received. Exiting..." << std::endl;
    running = false;
}

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

    nodar::zmq::TopbotPublisher publisher;
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
