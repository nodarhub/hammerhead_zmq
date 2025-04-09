#pragma once

#include <fstream>
#include <nodar/zmq/point_cloud.hpp>
#include <vector>

template <typename String>
inline void writePly(const String &filename, const std::vector<nodar::zmq::Point> &point_cloud, bool ascii = false) {
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
           "end_header\n";
    if (ascii) {
        for (const auto &pt : point_cloud) {
            out << pt.x << " " << pt.y << " " << pt.z << "\n";
        }
    } else {
        out.write(reinterpret_cast<const char *>(point_cloud.data()), point_cloud.size() * sizeof(nodar::zmq::Point));
    }
}