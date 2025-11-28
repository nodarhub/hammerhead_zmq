#pragma once

#include <cstring>
#include <iostream>
#include <memory>

#include "message_info.hpp"
#include "utils.hpp"

namespace nodar {
namespace zmq {

struct Velocity {
    [[nodiscard]] static constexpr auto msgSize() { return sizeof(Velocity) + sizeof(MessageInfo); }
    [[nodiscard]] static constexpr MessageInfo expected_info() { return MessageInfo(9); }

    uint64_t time{};  // timestamp in nanoseconds

    // Velocity in customer's coordinate system [m/s]
    float vx{};  // translation in x direction
    float vy{};  // translation in y direction
    float vz{};  // translation in z direction

    // Translation from customer coordinate system to Nodar coordinate system [m]
    float tx{};  // translation in x direction
    float ty{};  // translation in y direction
    float tz{};  // translation in z direction

    // Rotation quaternion from customer coordinate system to Nodar coordinate system
    // Quaternion format: [qw, qx, qy, qz] where qw is the scalar part
    float qw{1.0f};  // quaternion w component (scalar part)
    float qx{};  // quaternion x component
    float qy{};  // quaternion y component
    float qz{};  // quaternion z component

    Velocity() = default;

    explicit Velocity(uint64_t time_, float vx_, float vy_, float vz_, float tx_ = 0.0f, float ty_ = 0.0f,
                      float tz_ = 0.0f, float qw_ = 1.0f, float qx_ = 0.0f, float qy_ = 0.0f, float qz_ = 0.0f)
        : time(time_), vx(vx_), vy(vy_), vz(vz_), tx(tx_), ty(ty_), tz(tz_), qw(qw_), qx(qx_), qy(qy_), qz(qz_) {}

    explicit Velocity(const uint8_t *src) { read(src); }

    void read(const uint8_t *src) {
        // Check the info to make sure this message is the type we expect
        MessageInfo info;
        src = utils::read(src, info);
        if (info.is_different(expected_info(), "Velocity")) {
            std::cerr << "This message either is not a Velocity message, or is a different message version."
                      << std::endl;
            return;
        }

        // Read the basic data types
        src = utils::read(src, time);
        src = utils::read(src, vx);
        src = utils::read(src, vy);
        src = utils::read(src, vz);
        src = utils::read(src, tx);
        src = utils::read(src, ty);
        src = utils::read(src, tz);
        src = utils::read(src, qw);
        src = utils::read(src, qx);
        src = utils::read(src, qy);
        utils::read(src, qz);
    }

    static auto write(uint8_t *dst, uint64_t timestamp_ns, float vx, float vy, float vz, float tx, float ty, float tz,
                      float qw, float qx, float qy, float qz) {
        dst = utils::append(dst, expected_info());
        dst = utils::append(dst, timestamp_ns);
        dst = utils::append(dst, vx);
        dst = utils::append(dst, vy);
        dst = utils::append(dst, vz);
        dst = utils::append(dst, tx);
        dst = utils::append(dst, ty);
        dst = utils::append(dst, tz);
        dst = utils::append(dst, qw);
        dst = utils::append(dst, qx);
        dst = utils::append(dst, qy);
        return utils::append(dst, qz);
    }

    auto write(uint8_t *dst) const { return write(dst, time, vx, vy, vz, tx, ty, tz, qw, qx, qy, qz); }
};

}  // namespace zmq
}  // namespace nodar