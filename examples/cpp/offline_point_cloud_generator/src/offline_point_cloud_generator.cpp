#include <filesystem>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <vector>

#include "details.hpp"
#include "get_files.hpp"
#include "ply.hpp"
#include "point.hpp"
#include "safe_load.hpp"
#include "tqdm.hpp"

class PointCloudWriter {
public:
    void operator()(const std::string &ply_path, const Details &details, const cv::Mat &input_image,
                    const cv::Mat &left_rect, const bool &is_disparity) {
        // Allocate space for the point cloud
        std::vector<PointXYZRGB> point_cloud;
        const auto rows = left_rect.rows;
        const auto cols = left_rect.cols;
        point_cloud.resize(rows * cols);

        // Convert the input map to a point cloud
        if (is_disparity) {
            cv::reprojectImageTo3D(input_image, depth3d, details.projection);
        } else {
            const auto disparity = details.focal_length * details.baseline / input_image;
            cv::reprojectImageTo3D(disparity, depth3d, details.projection);
        }

        auto xyz = reinterpret_cast<float *>(depth3d.data);
        auto bgr = left_rect.data;
        const auto min_row = border;
        const auto max_row = rows - 1 - border;
        const auto min_col = border;
        const auto max_col = cols - 1 - border;
        size_t total = 0;
        size_t in_range = 0;
        size_t valid = 0;
        const auto downsample = 1;
        size_t num_points = 0;
        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < cols; ++col, xyz += 3, bgr += 3) {
                if (row > min_row and row < max_row and col > min_col and col < max_col and isValid(xyz)) {
                    ++valid;
                    if (inRange(xyz)) {
                        ++in_range;
                        if ((in_range % downsample) == 0) {
                            auto &point = point_cloud[num_points++];
                            point.x = -xyz[0], point.y = xyz[1], point.z = xyz[2];
                            point.b = bgr[0], point.g = bgr[1], point.r = bgr[2];
                        }
                    }
                }
                ++total;
            }
        }
        point_cloud.resize(num_points);
        if (false) {
            std::cout << num_points << " / " << total << " number of points used" << std::endl;
            std::cout << in_range << " / " << total << " in_range points" << std::endl;
            std::cout << valid << " / " << total << " valid points" << std::endl;
        }
        writePly(ply_path, point_cloud);
    }

private:
    static bool isValid(const float *const xyz) {
        return not std::isinf(xyz[0]) and not std::isinf(xyz[1]) and not std::isinf(xyz[2]);
    }

    bool inRange(const float *const xyz) const {
        const auto x = -xyz[0];
        const auto y = -xyz[1];
        const auto z = -xyz[2];
        return not(std::isinf(x)  //
                   or std::isinf(y) or y < y_min or y > y_max  //
                   or std::isinf(z) or z < z_min or z > z_max);
    }

    cv::Mat depth3d;
    size_t border = 8;
    float z_min = 0.5;
    float z_max = 500.0;
    float y_min = -50.0;
    float y_max = 50.0;
};

void processFiles(const std::vector<std::filesystem::path> &files, const std::filesystem::path &left_rect_dir,
                  const std::filesystem::path &details_dir, const std::filesystem::path &output_dir,
                  PointCloudWriter &point_cloud_writer, const bool &is_disparity) {
    for (const auto &file : tq::tqdm(files)) {
        auto input_image =
            safeLoad(file, is_disparity ? cv::IMREAD_ANYDEPTH : (cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH),
                     is_disparity ? CV_16UC1 : CV_32FC1, file, is_disparity ? "disparity image" : "depth image");

        if (input_image.empty()) {
            continue;
        }

        if (is_disparity && input_image.type() == CV_16UC1) {
            input_image.convertTo(input_image, CV_32FC1, 1.0 / 16.0);
        }

        const auto tiff = left_rect_dir / (file.stem().string() + ".tiff");
        const auto png = left_rect_dir / (file.stem().string() + ".png");
        const auto left_rect_filename = std::filesystem::exists(tiff) ? tiff : png;
        if (!std::filesystem::exists(left_rect_filename)) {
            std::cerr << "Could not find the corresponding left rectified image for\n"
                      << file << ". This path does not exist:\n"
                      << left_rect_filename << std::endl;
            continue;
        }
        const auto left_rect = safeLoad(left_rect_filename, cv::IMREAD_COLOR, CV_8UC3, file, "left rectified image");
        if (left_rect.empty()) {
            continue;
        }

        const auto details_filename = details_dir / (file.stem().string() + ".csv");
        if (!std::filesystem::exists(details_filename)) {
            std::cerr << "Could not find the corresponding details for\n"
                      << file << ". This path does not exist:\n"
                      << details_filename << std::endl;
            continue;
        }
        const Details details(details_filename);

        const auto ply_path = output_dir / (file.stem().string() + ".ply");
        point_cloud_writer(ply_path, details, input_image, left_rect, is_disparity);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Expecting at least one argument "
                  << "(the path to the recorded data). Usage:\n\n"
                  << "\toffline_point_cloud_generator data_directory [output_directory]" << std::endl;
        return EXIT_FAILURE;
    }
    const std::filesystem::path input_dir(argv[1]);
    const std::filesystem::path output_dir = argc > 2 ? argv[2] : (input_dir / "point_clouds");

    // Directories that we read

    // Mandatory directories
    const auto details_dir = input_dir / "details";
    const auto left_rect_dir = input_dir / "left-rect";

    // At least one of the disparity or depth directories is needed
    const auto disparity_dir = input_dir / "disparity";
    const auto depth_dir = input_dir / "depth";

    // Remove old output directory if it exists
    if (std::filesystem::exists(output_dir)) {
        // If you don't want to delete and overwrite old data, then set this bool to true
        if (false) {
            std::cerr << "Something already exists in the directory\n\t" << output_dir
                      << "\nDid you already generate this?\n"
                      << "If you want to rerun this tool on\n\t" << input_dir << "\nthen either delete the folder\n\t"
                      << output_dir << "\nor specify a different output_directory "
                      << "as the second argument.\nFor example:\n\t"
                      << "offline_point_cloud_generator " << input_dir << " output_directory" << std::endl;
            return EXIT_FAILURE;
        }
        std::filesystem::remove_all(output_dir);
    }
    std::filesystem::create_directories(output_dir);

    PointCloudWriter point_cloud_writer;
    if (std::filesystem::exists(disparity_dir)) {
        const auto disparities = getFiles(disparity_dir, ".tiff");
        std::cout << "Found " << disparities.size() << " disparity maps to convert to point clouds" << std::endl;
        processFiles(disparities, left_rect_dir, details_dir, output_dir, point_cloud_writer, true);
    } else if (std::filesystem::exists(depth_dir)) {
        const auto depths = getFiles(depth_dir, ".exr");
        std::cout << "Found " << depths.size() << " depth maps to convert to point clouds" << std::endl;
        processFiles(depths, left_rect_dir, details_dir, output_dir, point_cloud_writer, false);
    } else {
        std::cerr << "No disparity or depth data found in the input directory. Exiting." << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
