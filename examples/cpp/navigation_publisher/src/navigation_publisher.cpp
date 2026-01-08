#include "navigation_publisher.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

constexpr auto FRAME_RATE = 10;
std::atomic_bool running{true};

void signalHandler(int) {
    std::cerr << "SIGINT or SIGTERM received. Exiting..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    nodar::zmq::NavigationPublisher publisher;

    std::cout << "Publishing navigation data at " << FRAME_RATE << " Hz" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;

    while (running) {
        // Get current timestamp
        const auto timestamp =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        // Create NavigationData message
        nodar::zmq::NavigationData nav_data;
        nav_data.timestamp_ns = timestamp;

        // IMU data (example values)
        nav_data.imu.timestamp_ns = timestamp;
        nav_data.imu.acceleration_x_m_s2 = 0.0f;
        nav_data.imu.acceleration_y_m_s2 = 0.0f;
        nav_data.imu.acceleration_z_m_s2 = 9.81f;
        nav_data.imu.gyro_x_rad_s = 0.0f;
        nav_data.imu.gyro_y_rad_s = 0.0f;
        nav_data.imu.gyro_z_rad_s = 0.0f;
        nav_data.imu.magnetometer_x_gauss = 0.0f;
        nav_data.imu.magnetometer_y_gauss = 0.0f;
        nav_data.imu.magnetometer_z_gauss = 0.0f;
        nav_data.imu.temperature_degC = 25.0f;

        // GPS data (example values)
        nav_data.gps.timestamp_ns = timestamp;
        nav_data.gps.latitude_deg = 0.0f;
        nav_data.gps.longitude_deg = 0.0f;
        nav_data.gps.altitude_m = 10.0f;
        nav_data.gps.horizontal_uncertainty_m = 0.0f;
        nav_data.gps.vertical_uncertainty_m = 0.0f;
        nav_data.gps.speed_m_s = 5.0f;
        nav_data.gps.course_deg = 0.0f;
        nav_data.gps.fix_type = 0;
        nav_data.gps.num_satellites = 1;

        // Odometry data (example values - vehicle moving forward at 5 m/s in body frame)
        nav_data.odom.timestamp_ns = timestamp;
        nav_data.odom.position_x_m = 0.0f;
        nav_data.odom.position_y_m = 0.0f;
        nav_data.odom.position_z_m = 0.0f;
        nav_data.odom.velocity_x_m_s = 5.0f;  // 5 m/s forward
        nav_data.odom.velocity_y_m_s = 0.0f;  // no lateral motion
        nav_data.odom.velocity_z_m_s = 0.0f;  // no vertical motion
        nav_data.odom.angular_velocity_x_rad_s = 0.0f;
        nav_data.odom.angular_velocity_y_rad_s = 0.0f;
        nav_data.odom.angular_velocity_z_rad_s = 0.0f;

        // Transformation matrix from body frame to Nodar raw camera frame
        // Body frame: x=forward, y=left, z=up
        // Nodar raw camera frame: x=right, y=down, z=forward
        // Transform: Nodar_x = -Body_y, Nodar_y = -Body_z, Nodar_z = Body_x
        nav_data.T_body_to_raw_camera = {
            0.0f, -1.0f, 0.0f,  0.0f,  // Row 1: Nodar x = -Body y
            0.0f, 0.0f,  -1.0f, 0.0f,  // Row 2: Nodar y = -Body z
            1.0f, 0.0f,  0.0f,  0.0f,  // Row 3: Nodar z = Body x
            0.0f, 0.0f,  0.0f,  1.0f  // Row 4: Homogeneous
        };

        if (publisher.publishNavigation(nav_data)) {
            std::cout << "[" << timestamp << "] Publishing NavigationData:" << std::endl;
            std::cout << "  IMU: accel=(" << std::fixed << std::setprecision(2) << nav_data.imu.acceleration_x_m_s2
                      << ", " << nav_data.imu.acceleration_y_m_s2 << ", " << nav_data.imu.acceleration_z_m_s2
                      << ") m/s², gyro=(" << std::setprecision(3) << nav_data.imu.gyro_x_rad_s << ", "
                      << nav_data.imu.gyro_y_rad_s << ", " << nav_data.imu.gyro_z_rad_s << ") rad/s, mag=("
                      << std::setprecision(2) << nav_data.imu.magnetometer_x_gauss << ", "
                      << nav_data.imu.magnetometer_y_gauss << ", " << nav_data.imu.magnetometer_z_gauss
                      << ") gauss, temp=" << std::setprecision(1) << nav_data.imu.temperature_degC << "°C" << std::endl;
            std::cout << "  GPS: lat=" << std::setprecision(6) << nav_data.gps.latitude_deg
                      << "°, lon=" << nav_data.gps.longitude_deg << "°, alt=" << std::setprecision(2)
                      << nav_data.gps.altitude_m << "m, speed=" << nav_data.gps.speed_m_s
                      << "m/s, fix=" << nav_data.gps.fix_type << ", sats=" << nav_data.gps.num_satellites << std::endl;
            std::cout << "  Odom: pos=(" << nav_data.odom.position_x_m << ", " << nav_data.odom.position_y_m << ", "
                      << nav_data.odom.position_z_m << ")m, vel=(" << nav_data.odom.velocity_x_m_s << ", "
                      << nav_data.odom.velocity_y_m_s << ", " << nav_data.odom.velocity_z_m_s << ")m/s, ang_vel=("
                      << std::setprecision(3) << nav_data.odom.angular_velocity_x_rad_s << ", "
                      << nav_data.odom.angular_velocity_y_rad_s << ", " << nav_data.odom.angular_velocity_z_rad_s
                      << ")rad/s" << std::endl;
            std::cout << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds((1000 / FRAME_RATE)));
    }

    std::cout << "\nPublisher stopped." << std::endl;
    return EXIT_SUCCESS;
}