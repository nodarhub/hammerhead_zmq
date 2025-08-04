#include "topbot_publisher.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <optional>
#include <thread>
#include <unordered_map>

#include "get_files.hpp"
#include "nodar/zmq/topic_ports.hpp"
#include <yaml-cpp/yaml.h>

constexpr auto FRAME_RATE = 5;
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

std::optional<std::array<double, 6>> getOneExtrinsics(const std::string& image_file_path,
                                                      const std::string& extrinsics_dir) {
    namespace fs = std::filesystem;
    // Check if extrinsics directory exists
    if (!fs::exists(extrinsics_dir)) {
        std::cerr << "The provided extrinsics directory does not exist: " << extrinsics_dir << std::endl;
        return std::nullopt;
    }

    // Extract file name without extension
    fs::path img_path(image_file_path);
    std::string file_name_number_part = img_path.stem().string();

    // Build path to extrinsics YAML file
    fs::path extrinsics_file = fs::path(extrinsics_dir) / (file_name_number_part + ".yaml");
    if (!fs::exists(extrinsics_file)) {
        return std::nullopt;
    }

    // Load YAML
    YAML::Node extrinsics;
    try {
        extrinsics = YAML::LoadFile(extrinsics_file.string());
    } catch (const std::exception& e) {
        std::cerr << "Error loading YAML file: " << e.what() << std::endl;
        return std::nullopt;
    }

    // Required keys
    std::vector<std::string> ordered_required_keys = {
        "euler_x_deg", "euler_y_deg", "euler_z_deg", "Tx", "Ty", "Tz"
    };

    std::array<double, 6> result;
    size_t i = 0;
    for (const auto& key : ordered_required_keys) {
        if (!extrinsics[key]) {
            std::cerr << "Missing key '" << key << "' in extrinsics file: "
                      << extrinsics_file << std::endl;
            return std::nullopt;
        }
        result[i] = extrinsics[key].as<double>();
        i++;
    }

    return result;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::optional<std::string> pixel_format = std::nullopt;
    std::optional<std::string> extrinsics_dir = std::nullopt;
    bool wrong_input_format = argc < 3 || argc > 6;
    if (argc == 4) {
        pixel_format = argv[3];
    }
    if (argc == 5) {
        if (std::string(argv[3]) != "--extr") {
            wrong_input_format = true;
        }
        extrinsics_dir = argv[4];
    }
    if (argc == 6) {
        if (std::string(argv[3]) == "--extr") {
            extrinsics_dir = argv[4];
            pixel_format = argv[5];
        } else if (std::string(argv[4]) == "--extr") {
            pixel_format = argv[3];
            extrinsics_dir = argv[5];
        } else {
            wrong_input_format = true;
        }
    }
    if (wrong_input_format) {
        std::cerr << "Usage: topbot_publisher <topbot_data_directory> <port_number> [pixel_format] "
                  << "[--extr <extrinsics_directory>]" << std::endl;
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
        if (pixel_format.has_value()) {
            cvt_to_bgr_code = parsePixelFormat(pixel_format.value());
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Supported formats: BGR, Bayer_RGGB, Bayer_GRBG, Bayer_BGGR, Bayer_GBRG" << std::endl;
        return EXIT_FAILURE;
    }

    nodar::zmq::TopbotPublisher publisher(port);
    auto frame_id = 0;

    for (const auto& file : image_files) {
        if (!running)
            break;
        const auto img = cv::imread(file, cv::IMREAD_UNCHANGED);
        const auto timestamp =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        auto extrinsics = extrinsics_dir.has_value() ? getOneExtrinsics(file, extrinsics_dir.value()) : std::nullopt;
        if (publisher.publishImage(img, timestamp, frame_id, cvt_to_bgr_code, extrinsics)) {
            std::cout << "Published frame " << frame_id << " from " << file
                      << (extrinsics.has_value() ? " with extrinsics." : ".") << std::endl;
            frame_id++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FRAME_RATE));
    }

    std::cout << "Publisher stopped." << std::endl;
    return EXIT_SUCCESS;
}
