#include <filesystem>
#include <fstream>
#include <get_files.hpp>
#include <iostream>
#include <safe_load.hpp>
#include <tqdm.hpp>
#include <vector>

namespace {

std::vector<std::string> split_new_line(const std::string &source) {
    const std::string DELIMITER{"\n"};

    std::vector<std::string> result{};

    size_t start{0};
    size_t end{source.find(DELIMITER)};

    while (end != std::string::npos) {
        std::string line{source.substr(start, end - start)};

        if (!line.empty()) {
            result.push_back(std::move(line));
        }

        start = end + DELIMITER.length();
        end = source.find(DELIMITER, start);
    }

    // add the remaining part
    std::string line{source.substr(start)};

    if (!line.empty()) {
        result.push_back(std::move(line));
    }

    return result;
}

}  // namespace

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Expecting at least one argument "
                  << "(the path to the recorded data). Usage:\n\n"
                  << "\tlegacy_obstacle_data_converter data_directory [output_directory]" << std::endl;
        return EXIT_FAILURE;
    }
    const std::filesystem::path input_dir{argv[1]};
    const std::filesystem::path output_dir{argc > 2 ? argv[2] : (input_dir / "tracked-objects")};

    // Directories that we read
    const auto bbav_dir{input_dir / "bounding_boxes_and_velocities"};

    if (!std::filesystem::exists(bbav_dir)) {
        std::cerr << "Could not find the bounding_boxes_and_velocities directory: " << bbav_dir << "\n";
        return EXIT_FAILURE;
    }

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
    const auto bbavs{getFiles(input_dir / "bounding_boxes_and_velocities", ".csv")};
    std::cout << "Found " << bbavs.size() << " obstacle data files to convert" << std::endl;

    for (const auto &bbav : tq::tqdm(bbavs)) {
        const auto filename{bbav_dir / (bbav.stem().string() + ".csv")};
        if (not std::filesystem::exists(filename)) {
            std::cerr << "Could not find \n" << bbav << ". This path does not exist:\n" << filename << std::endl;
            continue;
        }

        std::ifstream file{filename, std::ios::in | std::ios::binary};
        if (!file.is_open()) {
            std::cerr << "Could not open " << filename << std::endl;
            continue;
        }

        file.seekg(0, std::ios::end);
        auto fileSize{static_cast<size_t>(file.tellg())};
        file.seekg(0, std::ios::beg);

        std::string content(fileSize, '\0');
        file.read(content.data(), static_cast<std::streamsize>(fileSize));

        auto lines{split_new_line(content)};

        if (lines.empty()) {
            continue;
        }

        const auto out_path{output_dir / (bbav.stem().string() + ".csv")};
        std::ofstream out{out_path};

        out << "id," << lines[0] << ",cellx, celly, ...\n";

        for (size_t i{1}; i < lines.size(); ++i) {
            out << std::to_string(i) << "," << lines[i] << ",\n";
        }
    }

    return 0;
}
