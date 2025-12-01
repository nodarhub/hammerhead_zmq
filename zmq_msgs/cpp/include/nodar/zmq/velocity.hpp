#pragma once

#include <array>
#include <cstring>
#include <iostream>
#include <memory>

#include "message_info.hpp"
#include "utils.hpp"

namespace nodar {
namespace zmq {

struct VelocityData {
    [[nodiscard]] static constexpr auto msgSize() { return sizeof(VelocityData) + sizeof(MessageInfo); }
    [[nodiscard]] static constexpr MessageInfo expected_info() { return MessageInfo(9); }

    uint64_t time{};  // timestamp in nanoseconds

    // Velocity in customer's coordinate system [m/s]
    std::array<float, 3> velocity{0.0f, 0.0f, 0.0f};  // [vx, vy, vz]

    // 3x3 Rotation matrix from customer coordinate system to Nodar coordinate system
    // Row-major order: [r00, r01, r02, r10, r11, r12, r20, r21, r22]
    std::array<float, 9> rotation{1.0f, 0.0f, 0.0f,  //
                                  0.0f, 1.0f, 0.0f,  //
                                  0.0f, 0.0f, 1.0f};

    VelocityData() = default;

    explicit VelocityData(uint64_t time_, const std::array<float, 3>& velocity_, const std::array<float, 9>& rotation_)
        : time(time_), velocity(velocity_), rotation(rotation_) {}

    explicit VelocityData(const uint8_t* src) { read(src); }

    void read(const uint8_t* src) {
        // Check the info to make sure this message is the type we expect
        MessageInfo info;
        src = utils::read(src, info);
        if (info.is_different(expected_info(), "VelocityData")) {
            std::cerr << "This message either is not a VelocityData message, or is a different message version."
                      << std::endl;
            return;
        }

        // Read timestamp
        src = utils::read(src, time);

        // Read velocity array
        for (size_t i = 0; i < 3; ++i) {
            src = utils::read(src, velocity[i]);
        }

        // Read rotation matrix array
        for (size_t i = 0; i < 9; ++i) {
            src = utils::read(src, rotation[i]);
        }
    }

    static auto write(uint8_t* dst, uint64_t timestamp_ns, const std::array<float, 3>& vel,
                      const std::array<float, 9>& rot) {
        dst = utils::append(dst, expected_info());
        dst = utils::append(dst, timestamp_ns);

        // Write velocity array
        for (size_t i = 0; i < 3; ++i) {
            dst = utils::append(dst, vel[i]);
        }

        // Write rotation matrix array
        for (size_t i = 0; i < 8; ++i) {
            dst = utils::append(dst, rot[i]);
        }
        return utils::append(dst, rot[8]);
    }

    auto write(uint8_t* dst) const { return write(dst, time, velocity, rotation); }
};

}  // namespace zmq
}  // namespace nodar