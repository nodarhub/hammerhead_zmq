#include "topbot_publisher.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <unordered_map>

#include "get_files.hpp"
#include "nodar/zmq/topic_ports.hpp"

constexpr auto FRAME_RATE = 10;
std::atomic_bool running{true};

void signalHandler(int) {
    std::cerr << "SIGINT or SIGTERM received. Exiting..." << std::endl;
    running = false;
}

// Function to validate port number
bool isValidPort(const uint16_t& port) {
    const auto reservedPorts = nodar::zmq::getReservedPorts();

    if (port < 1024 or port > 65535) {
        std::cerr << "Invalid port number: Port number must be in the range [1024, 65535]." << std::endl;
        return false;
    }
    if (reservedPorts.find(port) != reservedPorts.end()) {
        std::cerr << "Invalid port number: Port number is reserved. Please choose a port number other than 98xx."
                  << std::endl;
        return false;
    }
    return true;
}

// Function to parse pixel format string to OpenCV conversion code
uint8_t parsePixelFormat(const std::string& pixel_format) {
    static const std::unordered_map<std::string, uint8_t> pixel_format_map = {
        {"BGR", nodar::zmq::StampedImage::COLOR_CONVERSION::BGR2BGR},  // Default, no conversion needed
        {"Bayer_RGGB", cv::COLOR_BayerBG2BGR},
        {"Bayer_GRBG", cv::COLOR_BayerGB2BGR},
        {"Bayer_BGGR", cv::COLOR_BayerRG2BGR},
        {"Bayer_GBRG", cv::COLOR_BayerGR2BGR},
    };

    auto it = pixel_format_map.find(pixel_format);
    if (it == pixel_format_map.end()) {
        throw std::invalid_argument("Unsupported pixel format: " + pixel_format);
    }
    return it->second;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: topbot_publisher <topbot_data_directory> <port_number> [pixel_format]" << std::endl;
        std::cerr << "Supported pixel formats: BGR, Bayer_RGGB, Bayer_GRBG, Bayer_BGGR, Bayer_GBRG" << std::endl;
        return EXIT_FAILURE;
    }

    const auto image_files{getFiles(argv[1], ".tiff")};
    if (image_files.empty()) {
        std::cerr << "No images found in folder." << std::endl;
        return EXIT_FAILURE;
    }

    uint16_t port;
    try {
        const auto parsed_port = std::stoul(argv[2]);

        // Convert parsed value to uint16_t
        port = static_cast<uint16_t>(parsed_port);

        // Validate port number
        if (!isValidPort(port)) {
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        std::cerr << "Invalid port number: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Assume that the image doesn't need a transform.
    // Note that in the publisher.publishImage there is a check to see if the loaded image is roughly correct
    uint8_t cvt_to_bgr_code = nodar::zmq::StampedImage::COLOR_CONVERSION::BGR2BGR;
    try {
        if (argc == 4) {
            cvt_to_bgr_code = parsePixelFormat(argv[3]);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Supported formats: BGR, Bayer_RGGB, Bayer_GRBG, Bayer_BGGR, Bayer_GBRG" << std::endl;
        return EXIT_FAILURE;
    }

    nodar::zmq::TopbotPublisher publisher(port);
    auto frame_id = 0;

    while (running) {
        for (const auto& file : image_files) {
            if (!running)
                break;
            const auto img = cv::imread(file, cv::IMREAD_UNCHANGED);
            const auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();
            if (publisher.publishImage(img, timestamp, frame_id, cvt_to_bgr_code)) {
                std::cout << "Published frame " << frame_id << " from " << file << std::endl;
                frame_id++;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FRAME_RATE));
        }
    }

    std::cout << "Publisher stopped." << std::endl;
    return EXIT_SUCCESS;
}
