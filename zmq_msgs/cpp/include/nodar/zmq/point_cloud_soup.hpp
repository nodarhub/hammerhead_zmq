#pragma once

#include <iostream>
#include <utility>

#include "nodar/zmq/image.hpp"
#include "nodar/zmq/message_info.hpp"

namespace nodar {
namespace zmq {

struct PointCloudSoup {
    static constexpr MessageInfo getInfo() { return MessageInfo(1); }

    // Ideally INFO would be static constexpr, but to handle that nicely we would have to require C++17
    MessageInfo INFO = getInfo();
    uint64_t time{};
    uint64_t frame_id{};
    double baseline{};
    double focal_length{};
    std::array<float, 16> disparity_to_depth4x4{};
    static constexpr uint64_t disparity_to_depth4x4_bytes = 16 * sizeof(disparity_to_depth4x4[0]);
    StampedImage rectified;
    StampedImage disparity;

    PointCloudSoup() = default;

    PointCloudSoup(uint64_t time, uint64_t frame_id, double baseline, double focal_length,
                   std::array<float, 16> disparity_to_depth4x4, StampedImage rectified, StampedImage disparity)
        : time(time),
          frame_id(frame_id),
          baseline(baseline),
          focal_length(focal_length),
          disparity_to_depth4x4(disparity_to_depth4x4),
          rectified(std::move(rectified)),
          disparity(std::move(disparity)) {}

    explicit PointCloudSoup(const uint8_t *src) { read(src); }

    [[nodiscard]] static constexpr uint64_t msgSize(uint32_t rows_, uint32_t cols_, uint32_t rectified_type_,
                                                    uint32_t disparity_type_) {
        return sizeof(INFO) + sizeof(baseline) + sizeof(focal_length) + sizeof(time) + sizeof(frame_id) +
               disparity_to_depth4x4_bytes + StampedImage::msgSize(rows_, cols_, rectified_type_) +
               StampedImage::msgSize(rows_, cols_, disparity_type_);
    }

    [[nodiscard]] bool empty() const { return rectified.empty() or disparity.empty(); }

    [[nodiscard]] uint64_t msgSize() const {
        return msgSize(rectified.rows, rectified.cols, rectified.type, disparity.type);
    }

    void read(const uint8_t *src) {
        // Check the info to make sure this message is the type we expect
        MessageInfo info;
        src = utils::read(src, info);
        if (info != INFO) {
            std::cerr << "This message either is not a PointCloudSoup message, or is a different message version."
                      << std::endl;
            return;
        }

        // Read the basic data types
        src = utils::read(src, time);
        src = utils::read(src, frame_id);
        src = utils::read(src, baseline);
        src = utils::read(src, focal_length);
        memcpy(disparity_to_depth4x4.data(), src, disparity_to_depth4x4_bytes);
        src += disparity_to_depth4x4_bytes;

        // Read the image data
        rectified = StampedImage(src);
        src += rectified.msgSize();
        disparity = StampedImage(src);
        src += disparity.msgSize();
    }

    static auto write(uint8_t *dst, uint64_t time_, uint64_t frame_id_, double baseline_, double focal_length_,
                      std::array<float, 16> disparity_to_depth4x4_, uint32_t rows_, uint32_t cols_,
                      uint32_t rectified_type_, const uint8_t *rectified_data_, uint32_t disparity_type_,
                      const uint8_t *disparity_data_) {
        dst = utils::append(dst, getInfo());
        dst = utils::append(dst, time_);
        dst = utils::append(dst, frame_id_);
        dst = utils::append(dst, baseline_);
        dst = utils::append(dst, focal_length_);

        memcpy(dst, disparity_to_depth4x4_.data(), disparity_to_depth4x4_bytes);
        dst += disparity_to_depth4x4_bytes;

        dst = StampedImage::write(dst, time_, frame_id_, rows_, cols_, rectified_type_, rectified_data_);
        dst = StampedImage::write(dst, time_, frame_id_, rows_, cols_, disparity_type_, disparity_data_);
        return dst;
    }

    static auto write(uint8_t *dst, uint64_t time_, uint64_t frame_id_, double baseline_, double focal_length_,
                      std::array<float, 16> disparity_to_depth4x4_, const StampedImage &rectified_,
                      const StampedImage &disparity_) {
        assert(rectified_.rows == disparity_.rows &&
               "Rectified and disparity images must have the same number of rows");
        assert(rectified_.cols == disparity_.cols &&
               "Rectified and disparity images must have the same number of columns");
        return write(dst, time_, frame_id_, baseline_, focal_length_, disparity_to_depth4x4_, rectified_.rows,
                     rectified_.cols, rectified_.type, rectified_.img.data(), disparity_.type, disparity_.img.data());
    }

    auto write(uint8_t *dst) const {
        return write(dst, time, frame_id, baseline, focal_length, disparity_to_depth4x4, rectified, disparity);
    }
};

}  // namespace zmq
}  // namespace nodar