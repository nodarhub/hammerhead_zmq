#include "velocity_publisher.hpp"

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

    nodar::zmq::VelocityPublisher publisher;

    std::cout << "Publishing velocity data at " << FRAME_RATE << " Hz" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;

    while (running) {
        // Get current timestamp
        const auto timestamp =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        // Example velocity data in odometry coordinate system
        // Odometry system: x=forward, y=left, z=up
        std::array<float, 3> velocity = {5.0f,  // vx: 5 m/s forward
                                         0.0f,  // vy: no lateral motion
                                         0.0f};  // vz: no vertical motion

        // Rotation matrix from odometry to Nodar raw camera coordinate system
        // Nodar raw camera system: x=right, y=down, z=forward
        // Transform: Nodar_x = -Odom_y, Nodar_y = -Odom_z, Nodar_z = Odom_x
        std::array<float, 9> rotationToNodarRaw = {0.0f, -1.0f, 0.0f,  // Row 1: Nodar x = -Odom y
                                                   0.0f, 0.0f,  -1.0f,  // Row 2: Nodar y = -Odom z
                                                   1.0f, 0.0f,  0.0f};  // Row 3: Nodar z = Odom x

        if (publisher.publishVelocity(timestamp, velocity, rotationToNodarRaw)) {
            std::cout << "\rPublishing | vx: " << std::fixed << std::setprecision(2) << velocity[0]
                      << " m/s, vy: " << velocity[1] << " m/s, vz: " << velocity[2] << " m/s" << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds((1000 / FRAME_RATE)));
    }

    std::cout << "\nPublisher stopped." << std::endl;
    return EXIT_SUCCESS;
}