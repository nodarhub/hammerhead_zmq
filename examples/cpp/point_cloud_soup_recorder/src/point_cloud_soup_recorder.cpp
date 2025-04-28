#include <atomic>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/point_cloud_soup.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/calib3d.hpp>
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

    PointCloudSink(const std::filesystem::path &output_dir, const std::string &endpoint)
        : output_dir(output_dir), context(1), socket(context, ZMQ_SUB) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
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
        std::cout << "\rFrame # " << frame_id << ". " << std::flush;

        // Allocate space for the point cloud
        const auto rows = soup.disparity.rows;
        const auto cols = soup.disparity.cols;
        point_cloud.resize(rows * cols);

        // Disparity is in 11.6 format
        cv::Mat disparity_to_depth4x4(4, 4, CV_32FC1);
        memcpy(disparity_to_depth4x4.data, soup.disparity_to_depth4x4.data(), sizeof(soup.disparity_to_depth4x4));
        // Negate the last row of the Q-matrix
        disparity_to_depth4x4.row(3) = -disparity_to_depth4x4.row(3);
        auto disparity_scaled = nodar::zmq::cvMatFromStampedImage(soup.disparity);
        disparity_scaled.convertTo(disparity_scaled, CV_32F, 1. / 16);
        cv::reprojectImageTo3D(disparity_scaled, depth3d, disparity_to_depth4x4);

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
        std::cout << "Writing " << filename << std::flush;
        writePly(filename, point_cloud);
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
};

void printUsage(const std::string &default_ip) {
    std::cout << "You should specify the IP address of the device running hammerhead:\n\n"
                 "     ./point_cloud_soup_recorder hammerhead_ip\n\n"
                 "e.g. ./point_cloud_soup_recorder 192.168.1.9\n\n"
                 "In the meantime, we are going to assume that you are running this on the device running hammerhead,\n"
                 "that is, we assume that you specified\n\n"
                 "     ./point_cloud_soup_recorder "
              << default_ip << "\n----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    static constexpr auto default_ip = "127.0.0.1";
    static constexpr auto topic = nodar::zmq::SOUP_TOPIC;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc == 1) {
        printUsage(default_ip);
    }
    const auto ip = argc > 1 ? argv[1] : default_ip;
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(topic.port);

    const auto HERE = std::filesystem::path(__FILE__).parent_path();
    const auto output_dir = HERE / "point_clouds";
    std::filesystem::create_directories(output_dir);

    PointCloudSink sink(output_dir, endpoint);
    while (running) {
        sink.loopOnce();
    }
}