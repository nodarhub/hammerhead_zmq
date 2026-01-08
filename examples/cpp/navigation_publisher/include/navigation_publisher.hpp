#pragma once

#include <array>
#include <iostream>
#include <nodar/zmq/navigation.hpp>
#include <nodar/zmq/publisher.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <string>
#include <thread>

namespace nodar {
namespace zmq {

class NavigationPublisher {
public:
    explicit NavigationPublisher() : publisher(NAVIGATION_TOPIC, "") {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    bool publishNavigation(const nodar::zmq::NavigationData& nav_data) {
        auto buffer = publisher.getBuffer();
        buffer->resize(nav_data.msgSize());
        nav_data.write(buffer->data());
        publisher.send(buffer);
        return true;
    }

private:
    nodar::zmq::Publisher<nodar::zmq::NavigationData> publisher;
};

}  // namespace zmq
}  // namespace nodar