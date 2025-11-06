#include <atomic>
#include <chrono>
#include <csignal>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/imgcodecs.hpp>
#include <sstream>
#include <string>
#include <tiffio.h>
#include <unordered_map>
#include <zmq.hpp>

std::atomic_bool running{true};

void signalHandler(int signum) {
    std::cerr << "SIGINT or SIGTERM received." << std::endl;
    running = false;
}

const std::unordered_map<int, const char *> types = {
    {CV_8UC1, "CV_8UC1"},  //
    {CV_8UC3, "CV_8UC3"},  //
    {CV_8SC1, "CV_8SC1"},  //
    {CV_8SC3, "CV_8SC3"},  //
    {CV_16UC1, "CV_16UC1"},  //
    {CV_16UC3, "CV_16UC3"},  //
    {CV_16SC1, "CV_16SC1"},  //
    {CV_16SC3, "CV_16SC3"},  //
};

void printDetails(const cv::Mat &mat) {
    std::cout << mat.rows << ", " << mat.cols << ", " << mat.channels() << ", " << types.find(mat.type())->second
              << std::endl;
}

// Add timestamp metadata to TIFF file (compatible with Hammerhead viewer format)
bool add_tiff_metadata(const std::filesystem::path &filename, uint64_t left_time, uint64_t right_time) {
    TIFF *tiff = TIFFOpen(filename.c_str(), "r+");
    if (!tiff) {
        return false;
    }

    // Create YAML string with all required fields (compatible with Hammerhead format)
    // Must include all fields that DetailsParameters expects
    std::ostringstream details_str;
    details_str << "left_time: " << left_time << "\\n"
                << "right_time: " << right_time << "\\n"
                << "focal_length: 0.0\\n"
                << "baseline: 0.0\\n"
                << "meters_above_ground: 0.0\\n"
                << "projection: [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]\\n"
                << "rotation_disparity_to_raw_cam: [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]\\n"
                << "rotation_world_to_raw_cam: [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]\\n";

    std::ostringstream metadata_yaml;
    metadata_yaml << "details: \"" << details_str.str() << "\"";

    // Embed metadata in the SOFTWARE tag using libtiff
    bool success = TIFFSetField(tiff, TIFFTAG_SOFTWARE, metadata_yaml.str().c_str());
    TIFFClose(tiff);
    return success;
}

class FPS {
public:
    // Store timestamps for up to 100 frames
    explicit FPS(size_t maxlen = 100) : frame_times(), start_time(), total_frames(0), maxlen(maxlen) {}

    /** Call this once per frame. **/
    void tic() {
        auto now = Clock::now();
        if (total_frames == 0) {
            start_time = now;
        }
        frame_times.push_back(now);
        if (frame_times.size() > maxlen) {
            frame_times.pop_front();
        }
        ++total_frames;
    }

    /** Compute FPS over the last n frames (if enough data). **/
    [[nodiscard]] double fps_over_window(size_t n) const {
        if (frame_times.size() < 2) {
            return 0.0;
        }
        if (frame_times.size() < n) {
            n = frame_times.size();
        }
        const auto t1 = frame_times[frame_times.size() - n];
        const auto t2 = frame_times.back();
        const std::chrono::duration<double> delta = t2 - t1;
        return (t2 == t1) ? 0.0 : static_cast<double>(n - 1) / delta.count();
    }

    [[nodiscard]] auto str() const {
        const double fps_100 = fps_over_window(100);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << "fps_100: " << fps_100 << ", fps_inf: ";
        if (total_frames <= 1) {
            ss << 0.0;
        } else {
            const std::chrono::duration<double> total_time = frame_times.back() - start_time;
            ss << static_cast<double>(total_frames - 1) / total_time.count();
        }
        return ss.str();
    }

private:
    using Clock = std::chrono::steady_clock;
    std::deque<Clock::time_point> frame_times;
    Clock::time_point start_time;
    size_t total_frames;
    size_t maxlen;
};

class ZMQImageRecorder {
public:
    ZMQImageRecorder(const std::string &endpoint,  //
                     const std::filesystem::path &output_dir,  //
                     const std::string &image_dirname)
        : context(1),
          socket(context, ZMQ_SUB),
          image_dir(output_dir / image_dirname),
          timing_dir(output_dir / "times"),
          timing_file(output_dir / "times.txt") {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
        std::cout << "Creating the directory " << image_dir << std::endl;
        std::filesystem::create_directories(image_dir);
        std::cout << "Creating the directory " << timing_dir << std::endl;
        std::filesystem::create_directories(timing_dir);
        compression_params.push_back(cv::IMWRITE_TIFF_COMPRESSION);
        compression_params.push_back(1);  // No compression
    }

    [[nodiscard]] static std::string frame_string(uint64_t frame_no) {
        std::ostringstream oss;
        oss << std::setw(9) << std::setfill('0') << frame_no;
        return oss.str();
    }

    void loop_once() {
        fps.tic();
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        const nodar::zmq::StampedImage stamped_image(static_cast<uint8_t *>(msg.data()));
        auto img = nodar::zmq::cvMatFromStampedImage(stamped_image);
        if (img.empty()) {
            return;
        }
        const auto &frame_id = stamped_image.frame_id;
        std::cout << "\rFrame # " << frame_id << ", Last #: " << last_frame_id  //
                  << ", img.shape = " << img.rows << "x" << img.cols << "x" << img.channels()  //
                  << ", img.dtype = " << img.type() << ". ";
        std::cout << fps.str() << ". ";
        if (last_frame_id != 0 and frame_id != last_frame_id + 1) {
            std::cout << "Frames dropped: " << frame_id - last_frame_id - 1 << ". ";
        }
        std::cout << std::flush;
        last_frame_id = frame_id;

        // We recommend saving tiffs with no compression if the data rate is high.
        // Depending on the underlying image type, you might want to use stamped_image.cvt_to_bgr_code
        // to convert to BGR before saving.
        const auto frame_str = frame_string(frame_id);
        const auto tiff_path = image_dir / (frame_str + ".tiff");
        cv::imwrite(tiff_path, img, compression_params);

        // Extract right_time from additional_field if present (for topbot messages)
        uint64_t right_time = 0;
        if (stamped_image.additional_field_size == 8) {
            memcpy(&right_time, stamped_image.additional_field.data(), 8);
        }

        // Add timestamp metadata to TIFF file (for topbot images with dual timestamps)
        if (right_time != 0) {
            add_tiff_metadata(tiff_path, stamped_image.time, right_time);
        }

        {
            std::ofstream f(timing_dir / (frame_str + ".txt"));
            f << stamped_image.time;
            if (right_time != 0) {
                f << " " << right_time;
            }
            f << std::flush;
        }
        timing_file << frame_string(frame_id) << " " << stamped_image.time;
        if (right_time != 0) {
            timing_file << " " << right_time;
        }
        timing_file << std::endl;
    }

private:
    zmq::context_t context;
    zmq::socket_t socket;
    uint64_t last_frame_id = 0;
    std::filesystem::path image_dir;
    std::filesystem::path timing_dir;
    std::ofstream timing_file;
    std::vector<int> compression_params;
    FPS fps;
};

std::string get_folder_name(const std::string &topic_name) {
    using namespace nodar::zmq;
    if (topic_name == LEFT_RAW_TOPIC.name) {
        return "left_raw";
    }
    if (topic_name == RIGHT_RAW_TOPIC.name) {
        return "right_raw";
    }
    if (topic_name == LEFT_RECT_TOPIC.name) {
        return "left_rect";
    }
    if (topic_name == RIGHT_RECT_TOPIC.name) {
        return "right_rect";
    }
    if (topic_name == DISPARITY_TOPIC.name) {
        return "disparity";
    }
    if (topic_name == COLOR_BLENDED_DEPTH_TOPIC.name) {
        return "color_blended_depth";
    }
    if (topic_name == TOPBOT_RAW_TOPIC.name) {
        return "topbot_raw";
    }
    if (topic_name == TOPBOT_RECT_TOPIC.name) {
        return "topbot_rect";
    }
    return "images";
}

void print_usage(const std::string &default_ip, const std::string &default_port,
                 const std::string &default_output_dir) {
    std::cout << "You should specify the IP address of the ZMQ source (the device running Hammerhead), \n"
                 "the port number of the message that you want to listen to, \n"
                 "and the folder where you want the data to be saved:\n\n"
                 "     ./image_recorder hammerhead_ip port output_dir\n\n"
                 "e.g. ./image_recorder 192.168.1.9 9800 recorded_images\n\n"
                 "Alternatively, you can specify one of the image topic names in topic_ports.hpp of zmq_msgs...\n\n"
                 "e.g. ./image_recorder 192.168.1.9 nodar/right/image_raw recorded_images\n\n"
                 "If unspecified, we assume you are running this on the device running Hammerhead,\n"
                 "along with the defaults\n\n"
              << "     ./image_recorder " << default_ip << " " << default_port << " " << default_output_dir + "\n\n"
              << "Note that the list of topic/port mappings is in topic_ports.hpp in the zmq_msgs target.\n"
              << "----------------------------------------" << std::endl;
}

inline std::string date_string() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream date_ss;
    date_ss << std::put_time(std::localtime(&now), "%Y%m%d-%H%M%S");
    return date_ss.str();
}

int main(int argc, char *argv[]) {
    constexpr auto default_ip = "127.0.0.1";
    constexpr auto default_topic = nodar::zmq::IMAGE_TOPICS[0];
    const std::string default_port = std::to_string(default_topic.port);
    const auto dated_folder = date_string();
    const auto default_output_dir = "./" + dated_folder;

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc < 4) {
        print_usage(default_ip, default_port, default_output_dir);
    }
    const auto ip = argc > 1 ? argv[1] : default_ip;

    // If no second argument was provided, then assume the default topic.
    // Try to parse the second argument to see if you provided a port number.
    nodar::zmq::Topic topic = default_topic;
    if (argc >= 3) {
        // See if we can parse the second argument as a port number
        bool invalid_topic = true;
        size_t port = 0;
        std::istringstream iss(argv[2]);
        if (iss >> port) {
            for (const auto &image_topic : nodar::zmq::IMAGE_TOPICS) {
                if (port == image_topic.port) {
                    topic = image_topic;
                    invalid_topic = false;
                }
            }
            if (invalid_topic) {
                std::cerr << "It seems like you specified a port number " << port
                          << " that does not correspond to a port on which images are being published." << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            // It seems like you specified a topic name. Let's see if it is a valid image topic
            const std::string topic_name = argv[2];
            for (const auto &image_topic : nodar::zmq::IMAGE_TOPICS) {
                if (topic_name == image_topic.name) {
                    topic = image_topic;
                    invalid_topic = false;
                }
            }
            if (invalid_topic) {
                std::cerr << "It seems like you specified a topic name " << topic_name
                          << " that does not correspond to a topic on which images are being published." << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    const auto output_dir = argc >= 4 ? (std::string(argv[3]) + "/" + dated_folder) : default_output_dir;
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(topic.port);
    std::filesystem::create_directories(output_dir);
    ZMQImageRecorder subscriber(endpoint, output_dir, get_folder_name(topic.name));
    while (running) {
        subscriber.loop_once();
    }
}
