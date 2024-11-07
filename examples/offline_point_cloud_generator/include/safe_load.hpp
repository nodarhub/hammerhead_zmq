#pragma once

#include <filesystem>
#include <iostream>
#include <opencv2/imgcodecs.hpp>

inline auto safeLoad(const std::filesystem::path &filename, int read_mode, int pixel_type,
                     const std::filesystem::path &reference_exr, const char *image_type) {
    cv::Mat img;
    if (not std::filesystem::exists(filename)) {
        std::cerr << "Could not find the corresponding " << image_type << " for\n"
                  << reference_exr << ". This path does not exist:\n"
                  << filename << std::endl;
    } else {
        try {
            img = cv::imread(filename, read_mode);
            if (img.empty()) {
                std::cerr << "\nError loading " << filename << ". "
                          << "The loaded image is empty. Skipping." << std::endl;
                img = cv::Mat();
            } else if (img.type() != pixel_type) {
                std::cerr << "\nError loading " << filename << ". "
                          << "The " << image_type << " pixels are of type " << img.type()
                          << " and not the expected type (" << pixel_type << "). Skipping." << std::endl;
                img = cv::Mat();
            }
        } catch (...) {
            std::cerr << "\nError loading " << filename << std::endl;
        }
    }
    return img;
}
