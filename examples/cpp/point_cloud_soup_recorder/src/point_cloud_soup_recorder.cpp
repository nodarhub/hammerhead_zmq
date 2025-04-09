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
        : output_dir(output_dir), context(1), socket(context, ZMQ_SUB), window_name(endpoint) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
    }

    void loopOnce(size_t frame_index) {
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
        const auto min_row = border;
        const auto max_row = rows - 1 - border;
        const auto min_col = border;
        const auto max_col = cols - 1 - border;
        size_t total = 0;
        size_t in_range = 0;
        size_t valid = 0;
        const auto downsample = 10;
        size_t num_points = 0;
        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < cols; ++col, xyz += 3, bgr += bgr_step) {
                if (row > min_row and row < max_row and col > min_col and col < max_col and isValid(xyz)) {
                    ++valid;
                    if (inRange(xyz)) {
                        ++in_range;
                        if ((in_range % downsample) == 0) {
                            auto &point = point_cloud[num_points++];
                            point.x = -xyz[0], point.y = -xyz[1], point.z = -xyz[2];
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
                }
                ++total;
            }
        }
        point_cloud.resize(num_points);
        if (false) {
            std::cout << num_points << " / " << total << " number of points used" << std::endl;
            std::cout << in_range << " / " << total << " in_range points" << std::endl;
            std::cout << valid << " / " << total << " valid points" << std::endl;
        }

        // Note that this frame index represents the number of frames received.
        // The messages themselves have a frame_id that tracks how many frames have been produced.
        // Due to networking issues, it could be that there are dropped frames, resulting in these numbers being
        // different
        const auto filename = output_dir / (std::to_string(frame_index) + ".ply");
        std::cout << "Writing " << filename << std::flush;
        writePly(filename, point_cloud);
    }

private:
    static bool isValid(const float *const xyz) {
        return not std::isinf(xyz[0]) and not std::isinf(xyz[1]) and not std::isinf(xyz[2]);
    }

    bool inRange(const float *const xyz) const {
        const auto x = -xyz[0];
        const auto y = -xyz[1];
        const auto z = -xyz[2];
        return not(std::isinf(x)  //
                   or std::isinf(y) or y < y_min or y > y_max  //
                   or std::isinf(z) or z < z_min or z > z_max);
    }

    std::filesystem::path output_dir;
    cv::Mat disparity_scaled, depth3d;
    std::vector<PointXYZRGB> point_cloud;
    size_t border = 8;
    float z_min = 8.0;
    float z_max = 500.0;
    float y_min = -50.0;
    float y_max = 50.0;
    zmq::context_t context;
    zmq::socket_t socket;
    std::string window_name;
};

constexpr auto DEFAULT_IP = "127.0.0.1";

void printUsage() {
    std::cout << "You should specify the IP address of the device running hammerhead:\n\n"
                 "     ./point_cloud_soup_recorder hammerhead_ip\n\n"
                 "e.g. ./point_cloud_soup_recorder 192.168.1.9\n\n"
                 "In the meantime, we are going to assume that you are running this on the device running hammerhead,\n"
                 "that is, we assume that you specified\n\n"
                 "     ./point_cloud_soup_recorder "
              << DEFAULT_IP << "\n----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    static constexpr auto TOPIC = nodar::zmq::SOUP_TOPIC;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc == 1) {
        printUsage();
    }
    const auto ip = argc > 1 ? argv[1] : DEFAULT_IP;
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(TOPIC.port);

    const auto HERE = std::filesystem::path(__FILE__).parent_path();
    const auto output_dir = HERE / "point_clouds";
    std::filesystem::create_directories(output_dir);

    PointCloudSink sink(output_dir, endpoint);
    size_t frame_index = 0;
    while (running) {
        // Note that this frame index represents the number of frames received.
        // The messages themselves have a frame_id that tracks how many frames have been produced.
        // Due to networking issues, it could be that there are dropped frames, resulting in these numbers being
        // different
        sink.loopOnce(frame_index++);
    }
}