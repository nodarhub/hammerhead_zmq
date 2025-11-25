#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/point_cloud_soup.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/calib3d.hpp>
#include <string>
#include <thread>
#include <zmq.hpp>

#include "ply.hpp"
#include "point.hpp"

std::atomic_bool running{true};

void signalHandler(int signum) {
    std::cerr << "SIGINT or SIGTERM received." << std::endl;
    running = false;
}

class PointCloudSink {
public:
    uint64_t last_frame_id = 0;

    PointCloudSink(const std::filesystem::path &output_dir, const std::string &endpoint,
                   const std::string &scheduler_endpoint, bool enable_scheduler)
        : output_dir(output_dir), context(1), socket(context, ZMQ_SUB), enable_scheduler(enable_scheduler) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;

        // Connect to scheduler (wait server) if enabled
        if (enable_scheduler) {
            scheduler_socket = std::make_unique<zmq::socket_t>(context, ZMQ_DEALER);
            scheduler_socket->connect(scheduler_endpoint);
            std::cout << "Connected to scheduler at " << scheduler_endpoint << std::endl;
        }
    }

    void loopOnce() {
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        const nodar::zmq::PointCloudSoup soup(static_cast<uint8_t *>(msg.data()));

        // If the soup was not received correctly, return
        if (soup.empty()) {
            return;
        }

        // Warn if we dropped a frame
        const auto &frame_id = soup.frame_id;
        if (last_frame_id != 0 and frame_id != last_frame_id + 1) {
            std::cerr << (frame_id - last_frame_id - 1) << " frames dropped. Current frame ID : " << frame_id
                      << ", last frame ID: " << last_frame_id << std::endl;
        }
        last_frame_id = frame_id;
        std::cout << "\rFrame # " << frame_id << ". " << std::endl;

        // Allocate space for the point cloud
        const auto rows = soup.disparity.rows;
        const auto cols = soup.disparity.cols;
        point_cloud.resize(rows * cols);

        // Disparity is in 11.6 format
        cv::Mat disparity_to_depth4x4(4, 4, CV_32FC1);
        memcpy(disparity_to_depth4x4.data, soup.disparity_to_depth4x4.data(),
               soup.disparity_to_depth4x4.size() * sizeof(float));

        // Rotation disparity to raw cam
        cv::Mat rotation_disparity_to_raw_cam(3, 3, CV_32FC1);
        memcpy(rotation_disparity_to_raw_cam.data, soup.rotation_disparity_to_raw_cam.data(),
               soup.rotation_disparity_to_raw_cam.size() * sizeof(float));
        // Rotation world to raw cam
        cv::Mat rotation_world_to_raw_cam(3, 3, CV_32FC1);
        memcpy(rotation_world_to_raw_cam.data, soup.rotation_world_to_raw_cam.data(),
               soup.rotation_world_to_raw_cam.size() * sizeof(float));

        // Compute disparity_to_rotated_depth4x4 (rotated Q matrix)
        cv::Mat1f rotation_disparity_to_world_4x4 = cv::Mat::eye(4, 4, CV_32F);
        cv::Mat(rotation_world_to_raw_cam.t() * rotation_disparity_to_raw_cam)
            .convertTo(rotation_disparity_to_world_4x4(cv::Rect(0, 0, 3, 3)), CV_32F);
        cv::Mat disparity_to_rotated_depth4x4 = rotation_disparity_to_world_4x4 * disparity_to_depth4x4;

        // Negate the last row of the Q-matrix
        disparity_to_rotated_depth4x4.row(3) = -disparity_to_rotated_depth4x4.row(3);

        auto disparity_scaled = nodar::zmq::cvMatFromStampedImage(soup.disparity);
        disparity_scaled.convertTo(disparity_scaled, CV_32F, 1. / 16);
        cv::reprojectImageTo3D(disparity_scaled, depth3d, disparity_to_rotated_depth4x4);

        // Assert types before continuing
        assert(depth3d.type() == CV_32FC3);
        const auto rect_type = soup.rectified.type;
        assert(rect_type == CV_8UC3 or rect_type == CV_8SC3 or rect_type == CV_16UC3 or rect_type == CV_16SC3);

        auto xyz = reinterpret_cast<float *>(depth3d.data);
        const auto bgr_step = rect_type == CV_8UC3 or rect_type == CV_8SC3 ? 3 : 6;
        auto bgr = soup.rectified.img.data();
        size_t total = 0;
        size_t valid = 0;
        const auto downsample = 10;
        size_t num_points = 0;
        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < cols; ++col, xyz += 3, bgr += bgr_step) {
                ++total;
                if (not isValid(xyz)) {
                    continue;
                }
                ++valid;
                if (valid % downsample) {
                    continue;
                }
                auto &point = point_cloud[num_points++];
                point.x = xyz[0], point.y = xyz[1], point.z = xyz[2];
                if (rect_type == CV_8UC3 || rect_type == CV_8SC3) {
                    point.b = bgr[0];
                    point.g = bgr[1];
                    point.r = bgr[2];
                } else if (rect_type == CV_16UC3 || rect_type == CV_16SC3) {
                    const auto *bgr16 = reinterpret_cast<const uint16_t *>(bgr);
                    point.b = static_cast<uint8_t>(bgr16[0] / 257);
                    point.g = static_cast<uint8_t>(bgr16[1] / 257);
                    point.r = static_cast<uint8_t>(bgr16[2] / 257);
                }
            }
        }
        point_cloud.resize(num_points);
        if (false) {
            std::cout << num_points << " / " << total << " number of points used" << std::endl;
            std::cout << valid << " / " << total << " valid points" << std::endl;
        }

        std::ostringstream filename_ss;
        filename_ss << std::setw(9) << std::setfill('0') << frame_id << ".ply";
        const auto filename = output_dir / filename_ss.str();
        std::cout << "Writing " << filename << std::endl;
        writePly(filename, point_cloud);

        // Wait for scheduler request from hammerhead, then send reply (if enabled)
        if (enable_scheduler && scheduler_socket) {
            zmq::message_t scheduler_request;
            scheduler_socket->recv(scheduler_request, zmq::recv_flags::none);  // Blocking wait

            // Send reply to release hammerhead for next frame
            zmq::message_t reply(2);
            memcpy(reply.data(), "OK", 2);
            scheduler_socket->send(reply, zmq::send_flags::none);
        }
    }

private:
    static bool isValid(const float *const xyz) {
        return not std::isinf(xyz[0]) and not std::isinf(xyz[1]) and not std::isinf(xyz[2]);
    }

    std::filesystem::path output_dir;
    cv::Mat depth3d;
    std::vector<PointXYZRGB> point_cloud;
    zmq::context_t context;
    zmq::socket_t socket;
    std::unique_ptr<zmq::socket_t> scheduler_socket;
    bool enable_scheduler;
};

void printUsage(const std::string &default_ip) {
    std::cout << "Usage: ./point_cloud_soup_recorder [OPTIONS] [hammerhead_ip] [output_directory]\n\n"
                 "Options:\n"
                 "  -w, --wait-for-scheduler    Enable scheduler synchronization with hammerhead\n\n"
                 "Arguments:\n"
                 "  hammerhead_ip               IP address of the device running hammerhead (default: "
              << default_ip
              << ")\n"
                 "  output_directory            Directory to save PLY files (default: ./point_clouds)\n\n"
                 "Examples:\n"
                 "  ./point_cloud_soup_recorder 10.10.1.10 /tmp/ply_output\n"
                 "  ./point_cloud_soup_recorder -w 10.10.1.10 /tmp/ply_output\n"
                 "  ./point_cloud_soup_recorder --wait-for-scheduler\n"
                 "----------------------------------------"
              << std::endl;
}

int main(int argc, char *argv[]) {
    static constexpr auto default_ip = "127.0.0.1";
    static constexpr auto topic = nodar::zmq::SOUP_TOPIC;
    static constexpr auto wait_topic = nodar::zmq::WAIT_TOPIC;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Parse command-line arguments
    bool enable_scheduler = false;
    int positional_arg_index = 1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-w" || arg == "--wait-for-scheduler") {
            enable_scheduler = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(default_ip);
            return 0;
        } else {
            // First positional argument
            positional_arg_index = i;
            break;
        }
    }

    if (argc == 1) {
        printUsage(default_ip);
    }

    // Parse positional arguments (IP and output directory)
    std::string ip = default_ip;
    std::filesystem::path output_dir;

    if (positional_arg_index < argc) {
        ip = argv[positional_arg_index];
    }

    if (positional_arg_index + 1 < argc) {
        output_dir = argv[positional_arg_index + 1];
    } else {
        const auto HERE = std::filesystem::path(__FILE__).parent_path();
        output_dir = HERE / "point_clouds";
    }

    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(topic.port);
    const auto scheduler_endpoint = std::string("tcp://") + ip + ":" + std::to_string(wait_topic.port);
    std::filesystem::create_directories(output_dir);

    PointCloudSink sink(output_dir, endpoint, scheduler_endpoint, enable_scheduler);
    while (running) {
        sink.loopOnce();
    }
}