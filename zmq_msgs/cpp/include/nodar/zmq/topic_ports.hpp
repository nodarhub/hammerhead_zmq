#pragma once

#include <array>
#include <cstdint>
#include <set>

namespace nodar {
namespace zmq {

struct Topic {
    const char* name;
    uint16_t port;
};

constexpr std::array<Topic, 7> IMAGE_TOPICS{{{"nodar/left/image_raw", 9800},  //
                                             {"nodar/right/image_raw", 9801},  //
                                             {"nodar/left/image_rect", 9802},  //
                                             {"nodar/right/image_rect", 9803},  //
                                             {"nodar/disparity", 9804},  //
                                             {"nodar/color_blended_depth/image_raw", 9805},  //
                                             {"nodar/topbot_raw", 9813}}};

constexpr Topic SOUP_TOPIC{"nodar/point_cloud_soup", 9806};

constexpr Topic CAMERA_EXPOSURE_TOPIC = {"nodar/set_exposure", 9807};
constexpr Topic CAMERA_GAIN_TOPIC = {"nodar/set_gain", 9808};

constexpr Topic POINT_CLOUD_TOPIC{"nodar/point_cloud", 9809};
constexpr Topic POINT_CLOUD_RGB_TOPIC{"nodar/point_cloud_rgb", 9810};

constexpr Topic RECORDING_TOPIC{"nodar/recording", 9811};

constexpr Topic OBSTACLE_TOPIC{"nodar/obstacle", 9812};

constexpr Topic WAIT_TOPIC{"nodar/wait", 9814};

// Function to retrieve reserved ports dynamically
inline auto getReservedPorts() {
    std::set<uint16_t> reserved_ports;
    for (const auto& topic : nodar::zmq::IMAGE_TOPICS) {
        reserved_ports.insert(topic.port);
    }
    reserved_ports.insert(nodar::zmq::SOUP_TOPIC.port);
    reserved_ports.insert(nodar::zmq::CAMERA_EXPOSURE_TOPIC.port);
    reserved_ports.insert(nodar::zmq::CAMERA_GAIN_TOPIC.port);
    reserved_ports.insert(nodar::zmq::POINT_CLOUD_TOPIC.port);
    reserved_ports.insert(nodar::zmq::POINT_CLOUD_RGB_TOPIC.port);
    reserved_ports.insert(nodar::zmq::RECORDING_TOPIC.port);
    reserved_ports.insert(nodar::zmq::OBSTACLE_TOPIC.port);
    reserved_ports.insert(nodar::zmq::WAIT_TOPIC.port);
    return reserved_ports;
}

}  // namespace zmq
}  // namespace nodar
