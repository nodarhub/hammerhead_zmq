#pragma once

#include <array>
#include <cstring>
#include <iostream>
#include <memory>

#include "message_info.hpp"
#include "utils.hpp"

namespace nodar {
namespace zmq {

struct NavigationData {
    [[nodiscard]] static constexpr auto msgSize() { return sizeof(NavigationData) + sizeof(MessageInfo); }
    [[nodiscard]] static constexpr MessageInfo expected_info() { return MessageInfo(9); }

    uint64_t timestamp_ns{0};

    // IMU data (in body frame)
    struct ImuData {
        uint64_t timestamp_ns{0};
        float acceleration_x_m_s2{0.0f};
        float acceleration_y_m_s2{0.0f};
        float acceleration_z_m_s2{0.0f};
        float gyro_x_rad_s{0.0f};
        float gyro_y_rad_s{0.0f};
        float gyro_z_rad_s{0.0f};
        float magnetometer_x_gauss{0.0f};
        float magnetometer_y_gauss{0.0f};
        float magnetometer_z_gauss{0.0f};
        float temperature_degC{0.0f};
    } imu;

    // GPS data (in global WGS84 coordinates)
    struct GpsData {
        uint64_t timestamp_ns{0};
        float latitude_deg{0.0f};
        float longitude_deg{0.0f};
        float altitude_m{0.0f};
        float horizontal_uncertainty_m{0.0f};
        float vertical_uncertainty_m{0.0f};
        float speed_m_s{0.0f};
        float course_deg{0.0f};
        int32_t fix_type{0};
        int32_t num_satellites{0};
    } gps;

    // Odometry data (in body frame)
    struct OdometryData {
        uint64_t timestamp_ns{0};
        float position_x_m{0.0f};
        float position_y_m{0.0f};
        float position_z_m{0.0f};
        float velocity_x_m_s{0.0f};
        float velocity_y_m_s{0.0f};
        float velocity_z_m_s{0.0f};
        float angular_velocity_x_rad_s{0.0f};
        float angular_velocity_y_rad_s{0.0f};
        float angular_velocity_z_rad_s{0.0f};
    } odom;

    // 4x4 transformation matrix from body frame to Nodar raw camera frame (row-major)
    std::array<float, 16> T_body_to_raw_camera{1.0f, 0.0f, 0.0f, 0.0f,  //
                                               0.0f, 1.0f, 0.0f, 0.0f,  //
                                               0.0f, 0.0f, 1.0f, 0.0f,  //
                                               0.0f, 0.0f, 0.0f, 1.0f};

    NavigationData() = default;

    explicit NavigationData(const uint8_t* src) { read(src); }

    void read(const uint8_t* src) {
        // Check the info to make sure this message is the type we expect
        MessageInfo info;
        src = utils::read(src, info);
        if (info.is_different(expected_info(), "NavigationData")) {
            std::cerr << "This message either is not a NavigationData message, or is a different message version."
                      << std::endl;
            return;
        }

        // Read timestamp
        src = utils::read(src, timestamp_ns);

        // Read IMU data
        src = utils::read(src, imu.timestamp_ns);
        src = utils::read(src, imu.acceleration_x_m_s2);
        src = utils::read(src, imu.acceleration_y_m_s2);
        src = utils::read(src, imu.acceleration_z_m_s2);
        src = utils::read(src, imu.gyro_x_rad_s);
        src = utils::read(src, imu.gyro_y_rad_s);
        src = utils::read(src, imu.gyro_z_rad_s);
        src = utils::read(src, imu.magnetometer_x_gauss);
        src = utils::read(src, imu.magnetometer_y_gauss);
        src = utils::read(src, imu.magnetometer_z_gauss);
        src = utils::read(src, imu.temperature_degC);

        // Read GPS data
        src = utils::read(src, gps.timestamp_ns);
        src = utils::read(src, gps.latitude_deg);
        src = utils::read(src, gps.longitude_deg);
        src = utils::read(src, gps.altitude_m);
        src = utils::read(src, gps.horizontal_uncertainty_m);
        src = utils::read(src, gps.vertical_uncertainty_m);
        src = utils::read(src, gps.speed_m_s);
        src = utils::read(src, gps.course_deg);
        src = utils::read(src, gps.fix_type);
        src = utils::read(src, gps.num_satellites);

        // Read Odometry data
        src = utils::read(src, odom.timestamp_ns);
        src = utils::read(src, odom.position_x_m);
        src = utils::read(src, odom.position_y_m);
        src = utils::read(src, odom.position_z_m);
        src = utils::read(src, odom.velocity_x_m_s);
        src = utils::read(src, odom.velocity_y_m_s);
        src = utils::read(src, odom.velocity_z_m_s);
        src = utils::read(src, odom.angular_velocity_x_rad_s);
        src = utils::read(src, odom.angular_velocity_y_rad_s);
        src = utils::read(src, odom.angular_velocity_z_rad_s);

        // Read transformation matrix
        for (size_t i = 0; i < 16; ++i) {
            src = utils::read(src, T_body_to_raw_camera[i]);
        }
    }

    auto write(uint8_t* dst) const {
        dst = utils::append(dst, expected_info());
        dst = utils::append(dst, timestamp_ns);

        // Write IMU data
        dst = utils::append(dst, imu.timestamp_ns);
        dst = utils::append(dst, imu.acceleration_x_m_s2);
        dst = utils::append(dst, imu.acceleration_y_m_s2);
        dst = utils::append(dst, imu.acceleration_z_m_s2);
        dst = utils::append(dst, imu.gyro_x_rad_s);
        dst = utils::append(dst, imu.gyro_y_rad_s);
        dst = utils::append(dst, imu.gyro_z_rad_s);
        dst = utils::append(dst, imu.magnetometer_x_gauss);
        dst = utils::append(dst, imu.magnetometer_y_gauss);
        dst = utils::append(dst, imu.magnetometer_z_gauss);
        dst = utils::append(dst, imu.temperature_degC);

        // Write GPS data
        dst = utils::append(dst, gps.timestamp_ns);
        dst = utils::append(dst, gps.latitude_deg);
        dst = utils::append(dst, gps.longitude_deg);
        dst = utils::append(dst, gps.altitude_m);
        dst = utils::append(dst, gps.horizontal_uncertainty_m);
        dst = utils::append(dst, gps.vertical_uncertainty_m);
        dst = utils::append(dst, gps.speed_m_s);
        dst = utils::append(dst, gps.course_deg);
        dst = utils::append(dst, gps.fix_type);
        dst = utils::append(dst, gps.num_satellites);

        // Write Odometry data
        dst = utils::append(dst, odom.timestamp_ns);
        dst = utils::append(dst, odom.position_x_m);
        dst = utils::append(dst, odom.position_y_m);
        dst = utils::append(dst, odom.position_z_m);
        dst = utils::append(dst, odom.velocity_x_m_s);
        dst = utils::append(dst, odom.velocity_y_m_s);
        dst = utils::append(dst, odom.velocity_z_m_s);
        dst = utils::append(dst, odom.angular_velocity_x_rad_s);
        dst = utils::append(dst, odom.angular_velocity_y_rad_s);
        dst = utils::append(dst, odom.angular_velocity_z_rad_s);

        // Write transformation matrix
        for (size_t i = 0; i < 15; ++i) {
            dst = utils::append(dst, T_body_to_raw_camera[i]);
        }
        return utils::append(dst, T_body_to_raw_camera[15]);
    }
};

}  // namespace zmq
}  // namespace nodar