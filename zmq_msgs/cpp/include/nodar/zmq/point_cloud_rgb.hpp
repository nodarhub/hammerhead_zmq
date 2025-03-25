#pragma once

#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

#include "nodar/zmq/message_info.hpp"
#include "nodar/zmq/point_cloud.hpp"
#include "nodar/zmq/utils.hpp"

namespace nodar {
namespace zmq {

struct PointCloudRGB {
    static constexpr uint64_t HEADER_SIZE = 512;
    static constexpr MessageInfo getInfo() { return MessageInfo(5); }

    // Ideally INFO would be static constexpr, but to handle that nicely we would have to require C++17
    MessageInfo INFO = getInfo();
    uint64_t time{};
    uint64_t frame_id{};
    uint64_t num_points{0};
    std::vector<Point> points{};
    std::vector<Point> colors{};

    PointCloudRGB() = default;

    PointCloudRGB(uint64_t time, uint64_t frame_id, uint64_t num_points, const float *point_data,
                  const float *color_data)
        : time(time), frame_id(frame_id), num_points(num_points), points(num_points), colors(num_points) {
        memcpy(points.data(), point_data, pointCloudBytes());
        memcpy(colors.data(), color_data, pointCloudBytes());
    }

    PointCloudRGB(uint64_t time, uint64_t frame_id, const std::vector<Point> &points, const std::vector<Point> &colors)
        : time(time), frame_id(frame_id), points(points), colors(colors) {
        assert(points.size() == colors.size() && "Assertion points.size() == colors.size() failed");
        num_points = points.size();
    }

    explicit PointCloudRGB(const uint8_t *src) { read(src); }

    [[nodiscard]] static constexpr uint64_t pointCloudBytes(uint64_t num_points_) {
        return num_points_ * sizeof(Point);
    }

    [[nodiscard]] static constexpr uint64_t msgSize(uint64_t num_points_) {
        return HEADER_SIZE + 2 * pointCloudBytes(num_points_);
    }

    [[nodiscard]] bool empty() const { return num_points == 0; }

    [[nodiscard]] uint64_t pointCloudBytes() const { return pointCloudBytes(num_points); }

    [[nodiscard]] uint64_t msgSize() const { return msgSize(num_points); }

    void update(uint64_t time_, uint64_t frame_id_, uint64_t num_points_, const float *point_data_,
                const float *color_data_) {
        time = time_;
        frame_id = frame_id_;
        num_points = num_points_;
        points.resize(num_points_);
        memcpy(points.data(), point_data_, pointCloudBytes());
        colors.resize(num_points_);
        memcpy(colors.data(), color_data_, pointCloudBytes());
    }

    void read(const uint8_t *src) {
        auto point_mem = src + HEADER_SIZE;

        // Check the info to make sure this message is the type we expect
        MessageInfo info;
        src = utils::read(src, info);
        if (info != INFO) {
            std::cerr << "This message either is not a PointCloudRGB message, or is a different message version."
                      << std::endl;
            return;
        }

        // Read the basic data types
        src = utils::read(src, time);
        src = utils::read(src, frame_id);
        src = utils::read(src, num_points);

        // Read the point cloud
        points.resize(num_points);
        memcpy(points.data(), point_mem, pointCloudBytes());
        colors.resize(num_points);
        point_mem += pointCloudBytes();
        memcpy(colors.data(), point_mem, pointCloudBytes());
    }

    static auto write(uint8_t *dst, uint64_t time_, uint64_t frame_id_, uint64_t num_points_, const float *point_data_,
                      const float *color_data_) {
        auto point_mem = dst + HEADER_SIZE;
        std::memset(dst, 0, HEADER_SIZE);  // Zero the header before writing
        dst = utils::append(dst, getInfo());
        dst = utils::append(dst, time_);
        dst = utils::append(dst, frame_id_);
        dst = utils::append(dst, num_points_);
        const auto point_cloud_bytes = pointCloudBytes(num_points_);
        memcpy(point_mem, point_data_, point_cloud_bytes);
        point_mem += point_cloud_bytes;
        memcpy(point_mem, color_data_, point_cloud_bytes);
        return point_mem + point_cloud_bytes;
    }

    auto write(uint8_t *dst) const {
        return write(dst, time, frame_id, num_points, reinterpret_cast<const float *>(points.data()),
                     reinterpret_cast<const float *>(colors.data()));
    }
};

}  // namespace zmq
}  // namespace nodar