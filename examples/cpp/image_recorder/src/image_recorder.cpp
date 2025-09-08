#include <atomic>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/imgcodecs.hpp>
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

class ZMQImageRecorder {
public:
    uint64_t last_frame_id = 0;

    ZMQImageRecorder(const std::string &endpoint, const std::string &output_dirname)
        : context(1), socket(context, ZMQ_SUB), output_dir(output_dirname) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
        std::filesystem::create_directories(output_dir);
        compression_params.push_back(cv::IMWRITE_TIFF_COMPRESSION);
        compression_params.push_back(1);  // No compression
    }

    std::string frame_string(uint64_t frame_no) {
        std::ostringstream oss;
        oss << std::setw(9) << std::setfill('0') << frame_no << ".tiff";
        return oss.str();
    }

    void loopOnce() {
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        const nodar::zmq::StampedImage stamped_image(static_cast<uint8_t *>(msg.data()));
        auto img = nodar::zmq::cvMatFromStampedImage(stamped_image);
        if (img.empty()) {
            return;
        }
        const auto &frame_id = stamped_image.frame_id;
        if (last_frame_id != 0 and frame_id != last_frame_id + 1) {
            std::cerr << (frame_id - last_frame_id - 1) << " frames dropped. Current frame ID : " << frame_id
                      << ", last frame ID: " << last_frame_id << std::endl;
        }
        last_frame_id = frame_id;
        std::cout << "\rFrame # " << frame_id << std::flush;

        // We recommend saving tiffs with no compression if the data rate is high.
        // Depending on the underlying image type, you might want to use stamped_image.cvt_to_bgr_code
        // to convert to BGR before saving.
        cv::imwrite(output_dir / frame_string(frame_id), img, compression_params);
    }

private:
    zmq::context_t context;
    zmq::socket_t socket;
    std::filesystem::path output_dir;
    std::vector<int> compression_params;
};

void printUsage(const std::string &DEFAULT_IP, const std::string &DEFAULT_PORT, const std::string &DEFAULT_OUTPUT_DIR) {
    std::cout << "You should specify the IP address of the device running hammerhead, \n"
                 "the port of the message that you want to listen to, "
                 "and the folder where you want the data to be saved:\n\n"
                 "     ./image_recorder hammerhead_ip port output_dir\n\n"
                 "e.g. ./image_recorder 10.10.1.10 9800 recorded_images\n\n"
                 "Alternatively, you can specify one of the image topic names in topic_ports.hpp of zmq_msgs:"
                 "e.g. ./image_recorder 10.10.1.10 nodar/right/image_raw output_dir\n\n"
                 "In the meantime, we assume that you are running this on the device running Hammerhead,\n"
                 "and that you want the images on port 9800, that is, we assume that you specified\n\n"
                 "     ./image_recorder " +
                     DEFAULT_IP + " " + DEFAULT_PORT + " " + DEFAULT_OUTPUT_DIR + "\n\n"
              << "\n\nNote that the list of topic/port mappings is in topic_ports.hpp header in the zmq_msgs target."
              << "\n----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    constexpr auto DEFAULT_IP = "127.0.0.1";
    constexpr auto DEFAULT_TOPIC = nodar::zmq::IMAGE_TOPICS[0];
    const std::string DEFAULT_PORT = std::to_string(DEFAULT_TOPIC.port);
    constexpr auto DEFAULT_OUTPUT_DIR = "recorded_images";

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc < 4) {
        printUsage(DEFAULT_IP, DEFAULT_PORT, DEFAULT_OUTPUT_DIR);
    }
    const auto ip = argc > 1 ? argv[1] : DEFAULT_IP;

    // If no second argument was provided, then assume the default topic.
    // Try to parse the second argument to see if you provided a port number.
    nodar::zmq::Topic topic = DEFAULT_TOPIC;
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

    const auto output_dirname = argc >= 4 ? argv[3] : DEFAULT_OUTPUT_DIR;
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(topic.port);
    ZMQImageRecorder subscriber(endpoint, output_dirname);
    while (running) {
        subscriber.loopOnce();
    }
}
