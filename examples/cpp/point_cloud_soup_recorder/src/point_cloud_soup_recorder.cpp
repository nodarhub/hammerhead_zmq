#include <atomic>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/point_cloud_soup.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <zmq.hpp>

#include "date_string.hpp"
#include "details_parameters.hpp"
#include "frame_string.hpp"
#include "ply.hpp"
#include "point.hpp"

enum class OutputFormat { Ply, Raw };

std::atomic_bool running{true};

void signalHandler(int signum) {
    std::cerr << "SIGINT or SIGTERM received." << std::endl;
    running = false;
}

// Build a DetailsParameters from a PointCloudSoup. `rightTime`, `exposure`, `gain`,
// and `metersAboveGround` are not carried by the soup — they stay at the struct's
// default of 0 and are written out as such by DetailsParameters::save().
[[nodiscard]] DetailsParameters detailsFromSoup(const nodar::zmq::PointCloudSoup &soup) {
    DetailsParameters d;
    d.leftTime = soup.time;
    d.focalLength = static_cast<float>(soup.focal_length);
    d.baseline = static_cast<float>(soup.baseline);
    d.projection = soup.disparity_to_depth4x4;
    d.rotationDisparityToRawCam = soup.rotation_disparity_to_raw_cam;
    d.rotationWorldToRawCam = soup.rotation_world_to_raw_cam;
    return d;
}

class PointCloudSink {
public:
    uint64_t last_frame_id = 0;

    PointCloudSink(const std::filesystem::path &output_dir, const std::string &endpoint,
                   const std::string &scheduler_endpoint, bool enable_scheduler, OutputFormat format, int downsample)
        : output_dir(output_dir),
          context(1),
          socket(context, ZMQ_SUB),
          enable_scheduler(enable_scheduler),
          format(format),
          downsample(downsample < 1 ? 1 : downsample) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;

        details_dir = output_dir / "details";
        std::filesystem::create_directories(details_dir);
        if (format == OutputFormat::Raw) {
            disparity_dir = output_dir / "disparity";
            left_rect_dir = output_dir / "left-rect";
            std::filesystem::create_directories(disparity_dir);
            std::filesystem::create_directories(left_rect_dir);
            tiff_params.push_back(cv::IMWRITE_TIFF_COMPRESSION);
            tiff_params.push_back(1);  // No compression
        } else {
            point_cloud_dir = output_dir / "point_clouds";
            std::filesystem::create_directories(point_cloud_dir);
        }

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

        const auto frame_str = frameString(frame_id);
        detailsFromSoup(soup).save((details_dir / (frame_str + ".yaml")).string());

        if (format == OutputFormat::Raw) {
            writeRaw(soup, frame_str);
        } else {
            writePointCloud(soup, frame_str);
        }

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
    void writeRaw(const nodar::zmq::PointCloudSoup &soup, const std::string &frame_str) {
        auto disparity_img = nodar::zmq::cvMatFromStampedImage(soup.disparity);
        const auto rectified_img = nodar::zmq::cvMatFromStampedImage(soup.rectified);
        disparity_img.convertTo(disparity_img, CV_16U);
        const auto disparity_path = disparity_dir / (frame_str + ".tiff");
        const auto left_rect_path = left_rect_dir / (frame_str + ".tiff");
        std::cout << "Writing " << disparity_path << " and " << left_rect_path << std::endl;
        cv::imwrite(disparity_path.string(), disparity_img, tiff_params);
        cv::imwrite(left_rect_path.string(), rectified_img, tiff_params);
    }

    void writePointCloud(const nodar::zmq::PointCloudSoup &soup, const std::string &frame_str) {
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
        size_t valid = 0;
        size_t num_points = 0;
        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < cols; ++col, xyz += 3, bgr += bgr_step) {
                if (not isValid(xyz)) {
                    continue;
                }
                ++valid;
                if (downsample > 1 and (valid % downsample)) {
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

        const auto filename = point_cloud_dir / (frame_str + ".ply");
        std::cout << "Writing " << filename << std::endl;
        writePly(filename, point_cloud);
    }

    static bool isValid(const float *const xyz) {
        return not std::isinf(xyz[0]) and not std::isinf(xyz[1]) and not std::isinf(xyz[2]);
    }

    std::filesystem::path output_dir;
    std::filesystem::path details_dir;
    std::filesystem::path disparity_dir;
    std::filesystem::path left_rect_dir;
    std::filesystem::path point_cloud_dir;
    cv::Mat depth3d;
    std::vector<PointXYZRGB> point_cloud;
    zmq::context_t context;
    zmq::socket_t socket;
    std::unique_ptr<zmq::socket_t> scheduler_socket;
    bool enable_scheduler;
    OutputFormat format;
    int downsample;
    std::vector<int> tiff_params;
};

void printUsage(const std::string &default_ip) {
    std::cout << "Usage: ./point_cloud_soup_recorder [OPTIONS]\n\n"
                 "Options:\n"
                 "  --ip ADDR                   IP address of the device running hammerhead\n"
                 "                                (default: "
              << default_ip
              << ").\n"
                 "  --output-dir DIR            Base directory to save output into (default: current\n"
                 "                                working directory). A UTC-timestamped subfolder\n"
                 "                                (YYYYMMDD-HHMMSS) is appended so each run is isolated.\n"
                 "  -f, --format {ply,raw}      Output format (default: ply).\n"
                 "                                ply : reconstruct a 3D point cloud and write\n"
                 "                                      {frame_id}.ply. A details/{frame_id}.yaml\n"
                 "                                      sidecar (timestamps, baseline, focal length,\n"
                 "                                      projection matrices) is written alongside.\n"
                 "                                raw : skip reconstruction. Write the disparity and\n"
                 "                                      left rectified images plus details/{frame_id}.yaml\n"
                 "                                      into three sibling folders (disparity/, left-rect/,\n"
                 "                                      details/). This mirrors the layout produced by the\n"
                 "                                      main Hammerhead `recorder` tool and lets a downstream\n"
                 "                                      process do the 3D reconstruction offline, keeping\n"
                 "                                      CPU usage low on the recording host.\n"
                 "  --downsample N              [ply only] Keep every Nth valid point when writing the PLY.\n"
                 "                                Exists because writing every point can outpace disk\n"
                 "                                bandwidth at high frame rates and large image sizes.\n"
                 "                                Set 1 to keep all points; increase if you see dropped\n"
                 "                                frames. Default: 10.\n"
                 "  -w, --wait-for-scheduler    Enable scheduler synchronization with hammerhead.\n"
                 "  -h, --help                  Display this help message.\n\n"
                 "Examples:\n"
                 "  ./point_cloud_soup_recorder --ip 10.10.1.10 --output-dir /tmp/ply_output\n"
                 "  ./point_cloud_soup_recorder --ip 10.10.1.10 --format raw --output-dir /tmp/recording\n"
                 "  ./point_cloud_soup_recorder --downsample 1\n"
                 "  ./point_cloud_soup_recorder --ip 10.10.1.10 -w\n"
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
    OutputFormat format = OutputFormat::Ply;
    int downsample = 10;
    std::string ip = default_ip;
    std::filesystem::path base_dir{"."};

    auto require_value = [&](int &i, const std::string &flag) -> const char * {
        if (i + 1 >= argc) {
            std::cerr << "Missing value for " << flag << std::endl;
            std::exit(EXIT_FAILURE);
        }
        return argv[++i];
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-w" || arg == "--wait-for-scheduler") {
            enable_scheduler = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(default_ip);
            return 0;
        } else if (arg == "--ip") {
            ip = require_value(i, arg);
        } else if (arg == "--output-dir") {
            base_dir = require_value(i, arg);
        } else if (arg == "-f" || arg == "--format") {
            const std::string value = require_value(i, arg);
            if (value == "ply") {
                format = OutputFormat::Ply;
            } else if (value == "raw") {
                format = OutputFormat::Raw;
            } else {
                std::cerr << "Unknown --format value: " << value << " (expected 'ply' or 'raw')" << std::endl;
                return EXIT_FAILURE;
            }
        } else if (arg == "--downsample") {
            downsample = std::stoi(require_value(i, arg));
            if (downsample < 1) {
                std::cerr << "--downsample must be >= 1" << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(default_ip);
            return EXIT_FAILURE;
        }
    }

    const auto output_dir = base_dir / dateString();

    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(topic.port);
    const auto scheduler_endpoint = std::string("tcp://") + ip + ":" + std::to_string(wait_topic.port);
    std::filesystem::create_directories(output_dir);
    std::cout << "Writing output to " << output_dir << std::endl;

    PointCloudSink sink(output_dir, endpoint, scheduler_endpoint, enable_scheduler, format, downsample);
    while (running) {
        sink.loopOnce();
    }
}
