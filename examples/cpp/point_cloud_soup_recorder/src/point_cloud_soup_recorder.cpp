#include <atomic>
#include <condition_variable>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/point_cloud_soup.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <thread>
#include <zmq.hpp>

#include "date_string.hpp"
#include "details_parameters.hpp"
#include "fps.hpp"
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

// Build the 4x4 projection with the disparity-to-world rotation.
[[nodiscard]] cv::Mat buildRotatedQMatrix(const nodar::zmq::PointCloudSoup &soup) {
    cv::Mat disparity_to_depth4x4(4, 4, CV_32FC1);
    memcpy(disparity_to_depth4x4.data, soup.disparity_to_depth4x4.data(),
           soup.disparity_to_depth4x4.size() * sizeof(float));

    cv::Mat rotation_disparity_to_raw_cam(3, 3, CV_32FC1);
    memcpy(rotation_disparity_to_raw_cam.data, soup.rotation_disparity_to_raw_cam.data(),
           soup.rotation_disparity_to_raw_cam.size() * sizeof(float));
    cv::Mat rotation_world_to_raw_cam(3, 3, CV_32FC1);
    memcpy(rotation_world_to_raw_cam.data, soup.rotation_world_to_raw_cam.data(),
           soup.rotation_world_to_raw_cam.size() * sizeof(float));

    cv::Mat1f rotation_disparity_to_world_4x4 = cv::Mat::eye(4, 4, CV_32F);
    cv::Mat(rotation_world_to_raw_cam.t() * rotation_disparity_to_raw_cam)
        .convertTo(rotation_disparity_to_world_4x4(cv::Rect(0, 0, 3, 3)), CV_32F);
    cv::Mat disparity_to_rotated_depth4x4 = rotation_disparity_to_world_4x4 * disparity_to_depth4x4;
    disparity_to_rotated_depth4x4.row(3) = -disparity_to_rotated_depth4x4.row(3);
    return disparity_to_rotated_depth4x4;
}

// Walk every pixel of the disparity image, reproject into world space via the
// rotated Q matrix, and append the corresponding BGR-coloured point.
// Pixels with disparity == 0 (no stereo match) or degenerate projections are skipped.
// When `downsample > 1`, keep every Nth valid point.
void reprojectSoupToPoints(const nodar::zmq::PointCloudSoup &soup, const cv::Mat &rotated_Q, int downsample,
                           std::vector<PointXYZRGB> &out) {
    float Q[16];
    memcpy(Q, rotated_Q.data, sizeof(Q));

    const auto rows = soup.disparity.rows;
    const auto cols = soup.disparity.cols;
    const auto rect_type = soup.rectified.type;
    assert(soup.disparity.type == CV_16SC1);
    assert(rect_type == CV_8UC3 or rect_type == CV_8SC3 or rect_type == CV_16UC3 or rect_type == CV_16SC3);

    const auto *disparity_raw = reinterpret_cast<const int16_t *>(soup.disparity.img.data());
    const auto bgr_step = rect_type == CV_8UC3 or rect_type == CV_8SC3 ? 3 : 6;
    auto bgr = soup.rectified.img.data();

    size_t valid = 0;
    for (size_t v = 0; v < rows; ++v) {
        for (size_t u = 0; u < cols; ++u, ++disparity_raw, bgr += bgr_step) {
            const int16_t d_raw = *disparity_raw;
            if (d_raw == 0) {
                continue;
            }
            const float d = d_raw * (1.0f / 16.0f);
            const float w = Q[12] * u + Q[13] * v + Q[14] * d + Q[15];
            if (w == 0.0f) {
                continue;
            }
            const float inv_w = 1.0f / w;

            ++valid;
            if (downsample > 1 and (valid % downsample)) {
                continue;
            }

            const float x = (Q[0] * u + Q[1] * v + Q[2] * d + Q[3]) * inv_w;
            const float y = (Q[4] * u + Q[5] * v + Q[6] * d + Q[7]) * inv_w;
            const float z = (Q[8] * u + Q[9] * v + Q[10] * d + Q[11]) * inv_w;
            uint8_t b8, g8, r8;
            if (rect_type == CV_8UC3 || rect_type == CV_8SC3) {
                b8 = bgr[0];
                g8 = bgr[1];
                r8 = bgr[2];
            } else {
                const auto *bgr16 = reinterpret_cast<const uint16_t *>(bgr);
                b8 = static_cast<uint8_t>(bgr16[0] / 257);
                g8 = static_cast<uint8_t>(bgr16[1] / 257);
                r8 = static_cast<uint8_t>(bgr16[2] / 257);
            }
            out.emplace_back(x, y, z, r8, g8, b8);
        }
    }
}

class PointCloudSink {
public:
    uint64_t last_frame_id = 0;

    ~PointCloudSink() {
        {
            std::lock_guard<std::mutex> lock(guard);
            done = true;
        }
        cv.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
    }

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

        if (format == OutputFormat::Ply) {
            worker = std::thread([this] { workerLoop(); });
        }
    }

    void loopOnce() {
        fps.tic();
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        const nodar::zmq::PointCloudSoup soup(static_cast<uint8_t *>(msg.data()));

        // If the soup was not received correctly, return
        if (soup.empty()) {
            return;
        }

        const auto &frame_id = soup.frame_id;
        std::cout << "\rFrame # " << frame_id << ", Last #: " << last_frame_id  //
                  << ", format = " << (format == OutputFormat::Raw ? "raw" : "ply") << ". ";
        std::cout << fps.str() << ". ";
        if (last_frame_id != 0 and frame_id != last_frame_id + 1) {
            std::cout << "Frames dropped: " << frame_id - last_frame_id - 1 << ". ";
        }
        std::cout << std::flush;
        last_frame_id = frame_id;

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
        cv::imwrite(disparity_path.string(), disparity_img, tiff_params);
        cv::imwrite(left_rect_path.string(), rectified_img, tiff_params);
    }

    void writePointCloud(const nodar::zmq::PointCloudSoup &soup, const std::string &frame_str) {
        const auto rows = soup.disparity.rows;
        const auto cols = soup.disparity.cols;
        std::vector<PointXYZRGB> point_cloud;
        point_cloud.reserve(rows * cols);

        reprojectSoupToPoints(soup, buildRotatedQMatrix(soup), downsample, point_cloud);

        // Wait for the previous write to finish, hand off this frame, and wake the worker.
        {
            std::unique_lock<std::mutex> lock(guard);
            cv.wait(lock, [this] { return !has_work; });
            std::swap(point_cloud, write_buffer);
            pending_filename = point_cloud_dir / (frame_str + ".ply");
            has_work = true;
        }
        cv.notify_all();
    }

    void workerLoop() {
        std::vector<PointXYZRGB> local_buffer;
        std::filesystem::path local_filename;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(guard);
                cv.wait(lock, [this] { return has_work || done; });
                if (!has_work) {
                    return;  // done == true and queue is drained
                }
                std::swap(local_buffer, write_buffer);
                local_filename = std::move(pending_filename);
                has_work = false;
            }
            cv.notify_all();
            writePly(local_filename, local_buffer);
        }
    }

    std::filesystem::path output_dir;
    std::filesystem::path details_dir;
    std::filesystem::path disparity_dir;
    std::filesystem::path left_rect_dir;
    std::filesystem::path point_cloud_dir;
    zmq::context_t context;
    zmq::socket_t socket;
    std::unique_ptr<zmq::socket_t> scheduler_socket;
    bool enable_scheduler;
    OutputFormat format;
    int downsample;
    std::vector<int> tiff_params;
    FPS fps;

    // Background PLY writer: main thread fills a local point cloud, swaps it into
    // `write_buffer`, sets `has_work`, and the worker swaps `write_buffer` into its
    // own local and writes that to disk.
    std::thread worker;
    std::mutex guard;
    std::condition_variable cv;
    bool has_work = false;
    bool done = false;
    std::filesystem::path pending_filename;
    std::vector<PointXYZRGB> write_buffer;
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
                 "                                      (timestamps, baseline, focal length,\n"
                 "                                      projection matrices) is written alongside.\n"
                 "                                raw : skip reconstruction. Write the disparity and\n"
                 "                                      left rectified images plus details/{frame_id}.yaml\n"
                 "                                      into three sibling folders (disparity/, left-rect/,\n"
                 "                                      details/). Lets a downstream process do the 3D\n"
                 "                                      reconstruction offline, keeping CPU usage low on\n"
                 "                                      the recording host.\n"
                 "  --downsample N              [ply only] Keep every Nth valid point when writing the PLY.\n"
                 "                                Reduces output file size / disk bandwidth; does NOT reduce\n"
                 "                                CPU cost on the recorder (reprojection still runs on every\n"
                 "                                pixel). Use --format raw if you are CPU-bound. Default: 1\n"
                 "                                (keep all points); increase if you see dropped frames.\n"
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
    int downsample = 1;
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
