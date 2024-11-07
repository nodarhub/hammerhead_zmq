#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

inline auto getFiles(const std::filesystem::path &dirname, const std::string &ext) {
    std::vector<std::filesystem::path> files;
    for (const auto &filename : std::filesystem::directory_iterator(dirname)) {
        const auto path = filename.path();
        if (path.extension() == ext) {
            files.push_back(filename.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}
