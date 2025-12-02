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

constexpr Topic LEFT_RAW_TOPIC{"nodar/left/image_raw", 9800};
constexpr Topic RIGHT_RAW_TOPIC{"nodar/right/image_raw", 9801};
constexpr Topic LEFT_RECT_TOPIC{"nodar/left/image_rect", 9802};
constexpr Topic RIGHT_RECT_TOPIC{"nodar/right/image_rect", 9803};
constexpr Topic DISPARITY_TOPIC{"nodar/disparity", 9804};
constexpr Topic COLOR_BLENDED_DEPTH_TOPIC{"nodar/color_blended_depth/image_raw", 9805};
constexpr Topic TOPBOT_RAW_TOPIC{"nodar/topbot_raw", 9813};
constexpr Topic TOPBOT_RECT_TOPIC{"nodar/topbot_rect", 9823};
constexpr Topic OCCUPANCY_MAP_TOPIC{"nodar/occupancy_map", 9900};

constexpr std::array<Topic, 9> IMAGE_TOPICS{{
    LEFT_RAW_TOPIC,  //
    RIGHT_RAW_TOPIC,  //
    LEFT_RECT_TOPIC,  //
    RIGHT_RECT_TOPIC,  //
    DISPARITY_TOPIC,  //
    COLOR_BLENDED_DEPTH_TOPIC,  //
    TOPBOT_RAW_TOPIC,  //
    TOPBOT_RECT_TOPIC,  //
    OCCUPANCY_MAP_TOPIC,  //
}};

constexpr Topic SOUP_TOPIC{"nodar/point_cloud_soup", 9806};

constexpr Topic CAMERA_EXPOSURE_TOPIC{"nodar/set_exposure", 9807};
constexpr Topic CAMERA_GAIN_TOPIC{"nodar/set_gain", 9808};

constexpr Topic POINT_CLOUD_TOPIC{"nodar/point_cloud", 9809};
constexpr Topic POINT_CLOUD_RGB_TOPIC{"nodar/point_cloud_rgb", 9810};

constexpr Topic RECORDING_TOPIC{"nodar/recording", 9811};

constexpr Topic OBSTACLE_TOPIC{"nodar/obstacle", 9812};

constexpr Topic WAIT_TOPIC{"nodar/wait", 9814};

constexpr Topic QA_FINDINGS_TOPIC{"nodar/qa_findings", 9822};

constexpr Topic VELOCITY_TOPIC{"nodar/velocity", 9824};

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
    reserved_ports.insert(nodar::zmq::QA_FINDINGS_TOPIC.port);
    reserved_ports.insert(nodar::zmq::VELOCITY_TOPIC.port);
    return reserved_ports;
}

}  // namespace zmq
}  // namespace nodar
