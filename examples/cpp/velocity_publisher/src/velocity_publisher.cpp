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

        // Example velocity data (constant forward motion in Nodar coordinate system)
        const float vx = 0.0f;  // no lateral motion
        const float vy = 0.0f;  // no vertical motion
        const float vz = 5.0f;  // 5 m/s forward (z is forward in Nodar system)

        // Identity transformation (customer coordinate system = Nodar coordinate system)
        const float tx = 0.0f, ty = 0.0f, tz = 0.0f;
        const float qw = 1.0f, qx = 0.0f, qy = 0.0f, qz = 0.0f;

        if (publisher.publishVelocity(timestamp, vx, vy, vz, tx, ty, tz, qw, qx, qy, qz)) {
            std::cout << "\rPublishing | vx: " << std::fixed << std::setprecision(2) << vx << " m/s, vy: " << vy
                      << " m/s, vz: " << vz << " m/s" << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds((1000 / FRAME_RATE)));
    }

    std::cout << "\nPublisher stopped." << std::endl;
    return EXIT_SUCCESS;
}