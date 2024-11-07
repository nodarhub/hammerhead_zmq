#pragma once

#include <iostream>
#include <vector>

#include "message_info.hpp"
#include "utils.hpp"

namespace nodar {
namespace zmq {

struct StampedImage {
    static constexpr uint64_t HEADER_SIZE = 64;
    static constexpr MessageInfo getInfo() { return MessageInfo(0); }

    // Ensure Interoperability with OpenCV
    static constexpr uint32_t TYPE_CHANNEL_MAX = 512;
    static constexpr uint32_t TYPE_CHANNEL_SHIFT = 3;
    static constexpr uint32_t TYPE_CHANNEL_MASK = (TYPE_CHANNEL_MAX - 1) << TYPE_CHANNEL_SHIFT;
    static constexpr uint32_t TYPE_DEPTH_MAX = 1 << TYPE_CHANNEL_SHIFT;
    static constexpr uint32_t TYPE_DEPTH_MASK = TYPE_DEPTH_MAX - 1;

    // Ideally INFO would be static constexpr, but to handle that nicely we would have to require C++17
    MessageInfo INFO = getInfo();
    uint64_t time{};
    uint64_t frame_id{};
    uint32_t rows{};
    uint32_t cols{};
    uint32_t type{};  // For compatibility, this should be equivalent to cv::Mat::type(), e.g. CV_8UC3
    std::vector<uint8_t> img;

    StampedImage() = default;

    StampedImage(uint64_t time_arg, uint64_t frame_id_arg, uint32_t rows_arg, uint32_t cols_arg, uint32_t type_arg,
                 const uint8_t *data)
        : time(time_arg),
          frame_id(frame_id_arg),
          rows(rows_arg),
          cols(cols_arg),
          type(type_arg),
          img(data, data + dataSize()) {}

    explicit StampedImage(const uint8_t *src) {
        // The message has a header, followed by the image data
        auto header = src;
        const auto data = header + HEADER_SIZE;

        // Check the info to make sure this message is the type we expect
        MessageInfo info;
        header = utils::read(header, info);
        if (info != INFO) {
            std::cerr << "This message either is not an image message, or is a different message version." << std::endl;
            return;
        }
        header = utils::read(header, time);
        header = utils::read(header, frame_id);

        // Read the image size to make sure that it is something plausible
        header = utils::read(header, rows);
        header = utils::read(header, cols);
        if (rows * cols > 1e8) {
            std::cerr << "According to the message, the image has the impossibly large of dimensions " << rows << " x "
                      << cols << ". We are ignoring this message so that you don't run out of memory." << std::endl;
            return;
        }
        header = utils::read(header, type);

        // Copy the image data
        img = std::vector<uint8_t>(data, data + dataSize());
    }

    [[nodiscard]] static constexpr uint32_t channels(uint32_t type_) {
        return ((type_ & TYPE_CHANNEL_MASK) >> TYPE_CHANNEL_SHIFT) + 1;
    }

    [[nodiscard]] static constexpr uint32_t depthType(uint32_t type_) { return type_ & TYPE_DEPTH_MASK; }

    [[nodiscard]] static constexpr uint32_t elemSize(uint32_t type_) {
        auto depth_type = depthType(type_);
        if (depth_type == 0 || depth_type == 1) {  // CV_8U, CV_8S
            return 1;
        } else if (depth_type == 2 || depth_type == 3 || depth_type == 7) {  // CV_16U, CV_16S, CV_16F
            return 2;
        } else if (depth_type == 4 || depth_type == 5) {  // CV_32S, CV_32F
            return 4;
        } else if (depth_type == 6) {  // CV_64F
            return 8;
        }
        std::cerr << "Unrecognized image element type code " << depth_type << std::endl;
        return 0;
    }

    [[nodiscard]] static constexpr uint64_t dataSize(uint32_t rows_, uint32_t cols_, uint32_t type_) {
        return rows_ * cols_ * channels(type_) * elemSize(type_);
    }

    [[nodiscard]] static constexpr uint64_t msgSize(uint32_t rows_, uint32_t cols_, uint32_t type_) {
        return HEADER_SIZE + dataSize(rows_, cols_, type_);
    }

    [[nodiscard]] bool empty() const { return rows == 0 || cols == 0; }

    [[nodiscard]] uint32_t channels() const { return channels(type); }

    [[nodiscard]] uint32_t depthType() const { return depthType(type); }

    [[nodiscard]] uint32_t elemSize() const { return elemSize(type); }

    [[nodiscard]] uint64_t dataSize() const { return dataSize(rows, cols, type); }

    [[nodiscard]] uint64_t msgSize() const { return msgSize(rows, cols, type); }

    void update(uint64_t time_, uint64_t frame_id_, uint32_t rows_, uint32_t cols_, uint32_t type_,
                const uint8_t *data_) {
        time = time_;
        frame_id = frame_id_;
        rows = rows_;
        cols = cols_;
        type = type_;
        img.resize(dataSize());
        memcpy(img.data(), data_, dataSize());
    }

    static auto write(uint8_t *dst, uint64_t time_, uint64_t frame_id_, uint32_t rows_, uint32_t cols_, uint32_t type_,
                      const uint8_t *img) {
        // The message has a header, followed by the image data
        auto header = dst;
        const auto data = header + HEADER_SIZE;
        // Initialize the header with 0's and then fill with data
        std::memset(header, 0, HEADER_SIZE);
        header = utils::append(header, getInfo());
        header = utils::append(header, time_);
        header = utils::append(header, frame_id_);
        header = utils::append(header, rows_);
        header = utils::append(header, cols_);
        header = utils::append(header, type_);

        // Copy the image data
        const auto data_size = dataSize(rows_, cols_, type_);
        memcpy(data, img, data_size);

        // And return the pointer to the end of the written data
        return data + data_size;
    }

    auto write(uint8_t *dst) const { return write(dst, time, frame_id, rows, cols, type, img.data()); }
};

}  // namespace zmq
}  // namespace nodar