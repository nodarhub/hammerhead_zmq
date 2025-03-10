#pragma once

#include <fstream>
#include <vector>

struct PointXYZRGB {
    float x;
    float y;
    float z;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

template <typename String>
inline void writePly(const String &filename, const std::vector<nodar::zmq::Point> &points,
                     const std::vector<nodar::zmq::Point> &colors, bool ascii = false) {
    assert(points.size() == colors.size() && "points and colors must be the same size");

    std::vector<PointXYZRGB> point_cloud;
    point_cloud.reserve(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        const auto &point = points[i];
        const auto &color = colors[i];
        point_cloud.push_back({point.x,  //
                               point.y,  //
                               point.z,  //
                               static_cast<uint8_t>(color.x * 255),  //
                               static_cast<uint8_t>(color.y * 255),  //
                               static_cast<uint8_t>(color.z * 255)});
    }

    std::ofstream out(filename, std::ios::binary);
    out << "ply\n";
    if (ascii) {
        out << "format ascii 1.0\n";
    } else {
        out << "format binary_little_endian 1.0\n";
    }
    out << "element vertex " << points.size() << "\n";
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