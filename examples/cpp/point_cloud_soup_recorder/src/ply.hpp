#pragma once

#include <fstream>
#include <vector>

#include "point.hpp"

template <typename String>
inline void writePly(const String &filename, const std::vector<PointXYZRGB> &point_cloud, bool ascii = false) {
    std::ofstream out(filename, std::ios::binary);
    out << "ply\n";
    if (ascii) {
        out << "format ascii 1.0\n";
    } else {
        out << "format binary_little_endian 1.0\n";
    }
    out << "element vertex " << point_cloud.size() << "\n";
    out << "property float x\n"
           "property float y\n"
           "property float z\n"
           "property uchar red\n"
           "property uchar green\n"
           "property uchar blue\n";
    if (not ascii) {
        out << "property uchar alpha\n";
    }
    out << "end_header\n";
    if (ascii) {
        for (const auto &pt : point_cloud) {
            out << pt.x << " " << pt.y << " " << pt.z << " " << (uint16_t)pt.r << " " << (uint16_t)pt.g << " "
                << (uint16_t)pt.b << "\n";
        }
    } else {
        out.write(reinterpret_cast<const char *>(point_cloud.data()), point_cloud.size() * sizeof(PointXYZRGB));
    }
}