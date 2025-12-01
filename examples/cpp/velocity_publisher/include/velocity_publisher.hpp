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
        nodar::zmq::VelocityData velocity_data(timestamp_ns, velocity, rotation);

        auto buffer = publisher.getBuffer();
        buffer->resize(velocity_data.msgSize());
        velocity_data.write(buffer->data());
        publisher.send(buffer);
        return true;
    }

private:
    nodar::zmq::Publisher<nodar::zmq::VelocityData> publisher;
};

}  // namespace zmq
}  // namespace nodar