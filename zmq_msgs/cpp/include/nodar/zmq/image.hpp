#pragma once

#include <iostream>
#include <vector>

#include "message_info.hpp"
#include "utils.hpp"

namespace nodar {
namespace zmq {

struct StampedImage {
    enum COLOR_CONVERSION : uint8_t { BGR2BGR = 253, INCONVERTIBLE = 254, UNSPECIFIED = 255 };

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
    // Number of bytes between the start of consecutive rows, matching cv::Mat::step[0] (a.k.a. mat.step).
    // This is in BYTES, not elements, exactly like OpenCV's step (use step1() in OpenCV for elements).
    // For tightly packed rows, this should be: cols * channels * elemSize
    // For submatrices and on some TEGRA systems, we see GpuMat use a larger value
    uint32_t step{};
    uint8_t cvt_to_bgr_code{UNSPECIFIED};
    uint16_t additional_field_size{0};
    std::vector<uint8_t> img;
    std::vector<uint8_t> additional_field;

    StampedImage() = default;

    StampedImage(uint64_t time_arg,  //
                 uint64_t frame_id_arg,  //
                 uint32_t rows_arg,  //
                 uint32_t cols_arg,  //
                 uint32_t type_arg,  //
                 const uint8_t *data)
        : StampedImage(time_arg, frame_id_arg, rows_arg, cols_arg, type_arg, UNSPECIFIED, data) {}

    StampedImage(uint64_t time_arg,  //
                 uint64_t frame_id_arg,  //
                 uint32_t rows_arg,  //
                 uint32_t cols_arg,  //
                 uint32_t type_arg,  //
                 uint8_t cvt_to_bgr_code_arg,  //
                 const uint8_t *data)
        : time(time_arg),
          frame_id(frame_id_arg),
          rows(rows_arg),
          cols(cols_arg),
          type(type_arg),
          step(packedStep(cols_arg, type_arg)),
          cvt_to_bgr_code(cvt_to_bgr_code_arg),
          img(data, data + dataSize(rows_arg, cols_arg, type_arg, 0)),
          additional_field_size{0},
          additional_field{} {}

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
        header = utils::read(header, step);
        header = utils::read(header, cvt_to_bgr_code);
        header = utils::read(header, additional_field_size);

        // The step (row stride) must be at least a tightly packed row;
        // a smaller value would mean the payload is too small to hold the image and points to a corrupt message.
        const auto packed_step = StampedImage::packedStep(cols, type);
        if (step < packed_step) {
            std::cerr << "According to the message, the row step of " << step
                      << " bytes is smaller than a tightly packed row of " << packed_step
                      << " bytes, which is invalid. We are ignoring this message." << std::endl;
            return;
        }

        // Copy the image data, which occupies step * rows bytes (step already accounts for any row padding).
        const auto image_data_size = StampedImage::dataSize(rows, cols, type, 0, step);

        // Guard against an implausibly large payload (500 MB)
        if (image_data_size > 500ull * 1024 * 1024) {
            std::cerr << "According to the message, the image payload is " << image_data_size
                      << " bytes, which is implausibly large. We are ignoring this message so that you don't "
                         "run out of memory."
                      << std::endl;
            return;
        }
        img = std::vector<uint8_t>(data, data + image_data_size);

        if (additional_field_size > 1024) {
            std::cerr << "According to the message, the additional field has exceeded the maximum size of 1024 bytes. "
                      << "We are ignoring this message so that you don't run out of memory." << std::endl;
        } else if (additional_field_size > 0) {
            // Copy the additional field, if it exists
            const auto additional_data = data + image_data_size;
            additional_field = std::vector<uint8_t>(additional_data, additional_data + additional_field_size);
        } else {
            // initialize an empty additional field
            additional_field = std::vector<uint8_t>();
        }
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

    /// Number of bytes in a tightly packed row: cols * channels * elemSize.
    [[nodiscard]] static constexpr uint32_t packedStep(uint32_t cols_, uint32_t type_) {
        return cols_ * channels(type_) * elemSize(type_);
    }

    /// Payload size for an explicit row stride (in bytes). cols/type are unused here but kept so this
    /// overload mirrors the packed one below; pass packedStep(cols, type) if your rows are tightly packed.
    [[nodiscard]] static constexpr uint64_t dataSize(uint32_t rows_, uint32_t /*cols_*/, uint32_t /*type_*/,
                                                     uint16_t additional_field_size_, uint32_t step_) {
        return static_cast<uint64_t>(step_) * rows_ + additional_field_size_;
    }

    /// Payload size for tightly packed rows.
    [[nodiscard]] static constexpr uint64_t dataSize(uint32_t rows_, uint32_t cols_, uint32_t type_,
                                                     uint16_t additional_field_size_) {
        return dataSize(rows_, cols_, type_, additional_field_size_, packedStep(cols_, type_));
    }

    /// Message size for an explicit row stride (in bytes).
    [[nodiscard]] static constexpr uint64_t msgSize(uint32_t rows_, uint32_t cols_, uint32_t type_,
                                                    uint16_t additional_field_size_, uint32_t step_) {
        return HEADER_SIZE + dataSize(rows_, cols_, type_, additional_field_size_, step_);
    }

    /// Message size for tightly packed rows.
    [[nodiscard]] static constexpr uint64_t msgSize(uint32_t rows_, uint32_t cols_, uint32_t type_,
                                                    uint16_t additional_field_size_) {
        return msgSize(rows_, cols_, type_, additional_field_size_, packedStep(cols_, type_));
    }

    [[nodiscard]] bool empty() const { return rows == 0 || cols == 0; }

    [[nodiscard]] uint32_t channels() const { return channels(type); }

    [[nodiscard]] uint32_t depthType() const { return depthType(type); }

    [[nodiscard]] uint32_t elemSize() const { return elemSize(type); }

    [[nodiscard]] uint64_t dataSize() const { return dataSize(rows, cols, type, additional_field_size, step); }

    [[nodiscard]] uint64_t additionalFieldSize() const { return additional_field.size(); }

    [[nodiscard]] uint64_t msgSize() const { return msgSize(rows, cols, type, additional_field_size, step); }

    void update(uint64_t time_,  //
                uint64_t frame_id_,  //
                uint32_t rows_,  //
                uint32_t cols_,  //
                uint32_t type_,  //
                uint8_t cvt_to_bgr_code_,  //
                const uint8_t *data_) {
        time = time_;
        frame_id = frame_id_;
        rows = rows_;
        cols = cols_;
        type = type_;
        cvt_to_bgr_code = cvt_to_bgr_code_;
        // The data pointer is assumed tightly packed, so the stride is the packed row size.
        step = packedStep(cols_, type_);
        img.resize(dataSize());
        memcpy(img.data(), data_, dataSize());
    }

    void update(uint64_t time_,  //
                uint64_t frame_id_,  //
                uint32_t rows_,  //
                uint32_t cols_,  //
                uint32_t type_,  //
                const uint8_t *data_) {
        update(time_, frame_id_, rows_, cols_, type_, UNSPECIFIED, data_);
    }

    void updateAdditionalField(const uint8_t *data, uint16_t size) {
        if (size > 1024) {
            std::cerr << "Additional field size exceeds the maximum of 1024 bytes. Ignoring this update." << std::endl;
            return;
        }
        additional_field_size = size;
        additional_field.resize(size);
        memcpy(additional_field.data(), data, size);
    }

    // Only write the image header
    // Return a pointer to the end of the header (where the image data should start).
    static auto write_header(uint8_t *dst,  //
                             uint64_t time_,  //
                             uint64_t frame_id_,  //
                             uint32_t rows_,  //
                             uint32_t cols_,  //
                             uint32_t type_,  //
                             uint8_t cvt_to_bgr_code_,  //
                             uint16_t additional_field_size_,  //
                             uint32_t step_) {
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
        header = utils::append(header, step_);
        header = utils::append(header, cvt_to_bgr_code_);
        header = utils::append(header, additional_field_size_);
        return data;
    }

    // Only write the image header, setting image_type to UNSPECIFIED.
    // Return a pointer to the end of the header (where the image data should start).
    static auto write_header(uint8_t *dst,  //
                             uint64_t time_,  //
                             uint64_t frame_id_,  //
                             uint32_t rows_,  //
                             uint32_t cols_,  //
                             uint32_t type_,  //
                             uint16_t additional_field_size_,  //
                             uint32_t step_) {
        return write_header(dst, time_, frame_id_, rows_, cols_, type_, UNSPECIFIED, additional_field_size_, step_);
    }

    // Write everything.
    // Return a pointer to the end of the image data
    static auto write(uint8_t *dst,  //
                      uint64_t time_,  //
                      uint64_t frame_id_,  //
                      uint32_t rows_,  //
                      uint32_t cols_,  //
                      uint32_t type_,  //
                      uint8_t cvt_to_bgr_code_,  //
                      const uint8_t *img,  //
                      uint16_t additional_field_size_,  //
                      uint32_t step_,  //
                      const uint8_t *additional_field = nullptr) {
        // Write the image header
        auto data =
            write_header(dst, time_, frame_id_, rows_, cols_, type_, cvt_to_bgr_code_, additional_field_size_, step_);

        // Now write the image data excluding the additional field.
        const auto data_size = dataSize(rows_, cols_, type_, 0, step_);
        memcpy(data, img, data_size);
        data += data_size;

        if (additional_field_size_ > 0 && additional_field != nullptr) {
            // Write the additional field, if it exists
            memcpy(data, additional_field, additional_field_size_);
            data += additional_field_size_;
        } else if (additional_field_size_ > 0) {
            std::cerr << "Warning: Additional field size is non-zero but no data provided." << std::endl;
        }

        // And return the pointer to the end of the written data
        return data;
    }

    // Write everything, setting image_type to UNSPECIFIED
    // Return a pointer to the end of the image data
    static auto write(uint8_t *dst,  //
                      uint64_t time_,  //
                      uint64_t frame_id_,  //
                      uint32_t rows_,  //
                      uint32_t cols_,  //
                      uint32_t type_,  //
                      const uint8_t *img,  //
                      uint16_t additional_field_size_,  //
                      uint32_t step_,  //
                      const uint8_t *additional_field = nullptr) {
        return write(dst, time_, frame_id_, rows_, cols_, type_, UNSPECIFIED, img, additional_field_size_, step_,
                     additional_field);
    }

    // Only write the image header, setting image_type to UNSPECIFIED.
    // Return a pointer to the end of the header (where the image data should start).
    auto write_header(uint8_t *dst) const {
        return write_header(dst, time, frame_id, rows, cols, type, cvt_to_bgr_code, additional_field_size, step);
    }

    // Write everything, setting image_type to UNSPECIFIED
    // Return a pointer to the end of the image data
    auto write(uint8_t *dst) const {
        return write(dst, time, frame_id, rows, cols, type, cvt_to_bgr_code, img.data(), additional_field_size, step,
                     additional_field.data());
    }
};

}  // namespace zmq
}  // namespace nodar
