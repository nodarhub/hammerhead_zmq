#pragma once

#include <array>
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
    explicit VelocityPublisher() : publisher(VELOCITY_TOPIC, "") {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    bool publishVelocity(const uint64_t& timestamp_ns, const std::array<float, 3>& velocity,
                         const std::array<float, 9>& rotation = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                                                 1.0f}) {
        nodar::zmq::Velocity vel_msg(timestamp_ns, velocity, rotation);

        auto buffer = publisher.getBuffer();
        buffer->resize(vel_msg.msgSize());
        vel_msg.write(buffer->data());
        publisher.send(buffer);
        return true;
    }

private:
    nodar::zmq::Publisher<nodar::zmq::Velocity> publisher;
};

}  // namespace zmq
}  // namespace nodar