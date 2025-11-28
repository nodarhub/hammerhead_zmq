#pragma once

#include <iostream>
#include <nodar/zmq/publisher.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <nodar/zmq/velocity.hpp>
#include <string>
#include <thread>

namespace nodar {
namespace zmq {

class VelocityPublisher {
public:
    explicit VelocityPublisher(const std::string& ip = "") : publisher(VELOCITY_TOPIC, ip) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    bool publishVelocity(const uint64_t& timestamp_ns, const float vx, const float vy, const float vz,
                         const float tx = 0.0f, const float ty = 0.0f, const float tz = 0.0f, const float qw = 1.0f,
                         const float qx = 0.0f, const float qy = 0.0f, const float qz = 0.0f) {
        // Create velocity message
        nodar::zmq::Velocity velocity(timestamp_ns, vx, vy, vz, tx, ty, tz, qw, qx, qy, qz);

        auto buffer = publisher.getBuffer();
        buffer->resize(velocity.msgSize());
        velocity.write(buffer->data());
        publisher.send(buffer);
        return true;
    }

private:
    nodar::zmq::Publisher<nodar::zmq::Velocity> publisher;
};

}  // namespace zmq
}  // namespace nodar