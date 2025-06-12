#include <details_parameters.hpp>
#include <filesystem>
#include <get_files.hpp>
#include <iostream>
#include <safe_load.hpp>
#include <tqdm.hpp>
#include <vector>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Expecting at least one argument "
                  << "(the path to the recorded data). Usage:\n\n"
                  << "\tdepth_to_disparity data_directory [output_directory]" << std::endl;
        return EXIT_FAILURE;
    }
    const std::filesystem::path input_dir{argv[1]};
    const std::filesystem::path output_dir{argc > 2 ? argv[2] : (input_dir / "disparity")};

    // Directories that we read
    const auto depth_dir{input_dir / "depth"};
    const auto details_dir{input_dir / "details"};

    // Remove old output directory if it exists
    if (std::filesystem::exists(output_dir)) {
        // If you don't want to delete and overwrite old data, then set this bool to true
        if (false) {
            std::cerr << "Something already exists in the directory\n\t" << output_dir
                      << "\nDid you already generate this?\n"
                      << "If you want to rerun this tool on\n\t" << input_dir << "\nthen either delete the folder\n\t"
                      << output_dir << "\nor specify a different output_directory "
                      << "as the second argument.\nFor example:\n\t"
                      << "depth_to_disparity " << input_dir << " output_directory" << std::endl;
            return EXIT_FAILURE;
        }
        std::filesystem::remove_all(output_dir);
    }

    std::filesystem::create_directories(output_dir);

    // Load the depth data
    auto tiffs{getFiles(input_dir / "depth", ".tiff")};
    const auto exrs{getFiles(input_dir / "depth", ".exr")};

    // If there are no tiffs, but there are exrs, we need to convert them to tiffs as a one-time upgrade
    if (tiffs.empty() && !exrs.empty()) {
        std::cout << "Legacy .exr files detected, converting .exr files to .tiff files..." << std::endl;
        std::vector<int> compression_params = {cv::IMWRITE_TIFF_COMPRESSION, 1};

        for (const auto &exr : tq::tqdm(exrs)) {
            const auto depthImage{safeLoad(exr, cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH, CV_32FC1, "depth image")};

            if (depthImage.empty()) {
                continue;
            }

            const auto filePath{depth_dir / (exr.stem().string() + ".tiff")};
            cv::imwrite(filePath, depthImage, compression_params);
        }

        // Reload the tiffs
        tiffs = getFiles(input_dir / "depth", ".tiff");
    }

    std::cout << "Found " << exrs.size() << " depth maps to convert to disparities" << std::endl;

    for (const auto &tiff : tq::tqdm(tiffs)) {  // Safely load all the images.
        const auto depth_image{safeLoad(tiff, cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH, CV_32FC1, "depth image")};
        if (depth_image.empty()) {
            continue;
        }

        // Load the details
        const auto details_filename{details_dir / (tiff.stem().string() + ".yaml")};
        if (not std::filesystem::exists(details_filename)) {
            std::cerr << "Could not find the corresponding details for\n"
                      << tiff << ". This path does not exist:\n"
                      << details_filename << std::endl;
            continue;
        }

        if (details_filename.extension() != ".yaml") {
            std::cerr << "The details file is not a .yaml file:\n"
                      << details_filename << "\n"
                      << "Please validate the data folder with the NodarViewer application." << std::endl;
            continue;
        }

        DetailsParameters details{};
        bool hasErrors{false};
        if (!details.parse(details_filename, hasErrors)) {
            std::cerr << "Could not parse the details file:\n" << details_filename << std::endl;
            continue;
        }

        if (hasErrors) {
            std::cerr << "The details file has errors:\n"
                      << details_filename << "\n"
                      << "Please validate the data folder with the NodarViewer application." << std::endl;
            continue;
        }

        // Generate disparity and write it to disk as .tiff files

        std::vector<int> compression_params{};  // Compression parameters
        compression_params.push_back(cv::IMWRITE_TIFF_COMPRESSION);
        compression_params.push_back(1);  // No compression

        cv::Mat img_disparity{cv::Mat((16.0f * details.focalLength * details.baseline) / depth_image)};
        img_disparity.convertTo(img_disparity, CV_16UC1);

        const auto file_path{output_dir / (tiff.stem().string() + ".tiff")};
        cv::imwrite(file_path, img_disparity, compression_params);
    }
    return 0;
}
