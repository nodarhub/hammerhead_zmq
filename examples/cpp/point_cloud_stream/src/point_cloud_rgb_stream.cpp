#include <atomic>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <nodar/zmq/point_cloud_rgb.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <zmq.hpp>

#include "ply_rgb.hpp"

std::atomic_bool running{true};

void signalHandler(int signum) {
    std::cerr << "SIGINT or SIGTERM received." << std::endl;
    running = false;
}

class PointCloudRGBSink {
public:
    uint64_t last_frame_id = 0;

    PointCloudRGBSink(const std::filesystem::path &output_dir, const std::string &endpoint)
        : output_dir(output_dir), context(1), socket(context, ZMQ_SUB) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
    }

    void loopOnce(size_t frame_index) {
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        point_cloud_rgb.read(static_cast<uint8_t *>(msg.data()));

        // If the point_cloud_rgb was not received correctly, return
        if (point_cloud_rgb.empty()) {
            return;
        }

        // Warn if we dropped a frame
        const auto &frame_id = point_cloud_rgb.frame_id;
        if (last_frame_id != 0 and frame_id != last_frame_id + 1) {
            std::cerr << (frame_id - last_frame_id - 1) << " frames dropped. Current frame ID : " << frame_id
                      << ", last frame ID: " << last_frame_id << std::endl;
        }
        last_frame_id = frame_id;
        std::cout << "\rFrame # " << frame_id << ". " << std::flush;

        // Note that this frame index represents the number of frames received.
        // The messages themselves have a frame_id that tracks how many frames have been produced.
        // Due to networking issues, it could be that there are dropped frames, resulting in these numbers being
        // different
        const auto filename = output_dir / (std::to_string(frame_index) + ".ply");
        std::cout << "Writing " << filename << std::flush;
        writePly(filename, point_cloud_rgb.points, point_cloud_rgb.colors);
    }

private:
    std::filesystem::path output_dir;
    nodar::zmq::PointCloudRGB point_cloud_rgb;
    zmq::context_t context;
    zmq::socket_t socket;
};

void printUsage(const std::string &default_ip) {
    std::cout << "You should specify the IP address of the device running hammerhead:\n\n"
                 "     ./point_cloud_rgb_stream hammerhead_ip\n\n"
                 "e.g. ./point_cloud_rgb_stream 192.168.1.9\n\n"
                 "In the meantime, we are going to assume that you are running this on the device running hammerhead,\n"
                 "that is, we assume that you specified\n\n     ./point_cloud_rgb_stream "
              << default_ip << "\n----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    static constexpr auto default_ip = "127.0.0.1";
    static constexpr auto topic = nodar::zmq::POINT_CLOUD_RGB_TOPIC;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc == 1) {
        printUsage(default_ip);
    }
    const auto ip = argc > 1 ? argv[1] : default_ip;
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(topic.port);

    const auto HERE = std::filesystem::path(__FILE__).parent_path();
    const auto output_dir = HERE / "point_clouds_rgb";
    std::filesystem::create_directories(output_dir);

    PointCloudRGBSink sink(output_dir, endpoint);
    size_t frame_index = 0;
    while (running) {
        // Note that this frame index represents the number of frames received.
        // The messages themselves have a frame_id that tracks how many frames have been produced.
        // Due to networking issues, it could be that there are dropped frames, resulting in these numbers being
        // different
        sink.loopOnce(frame_index++);
    }
}