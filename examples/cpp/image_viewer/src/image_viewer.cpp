#include <atomic>
#include <csignal>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/opencv_utils.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/highgui.hpp>
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

class ZMQImageViewer {
public:
    uint64_t last_frame_id = 0;

    ZMQImageViewer(const std::string &endpoint) : context(1), socket(context, ZMQ_SUB), window_name(endpoint) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
        cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    }

    void loopOnce() {
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        const nodar::zmq::StampedImage stamped_image(static_cast<uint8_t *>(msg.data()));
        auto img = nodar::zmq::cvMatFromStampedImage(stamped_image);
        if (img.type() == CV_16SC1) {
            // Highgui produces a strange-looking output for signed 16-bit images. Convert to unsigned
            img.convertTo(img, CV_16UC1);
        }
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

        // Downsize the image before viewing
        cv::resizeWindow(window_name, {640, 480});
        cv::imshow(window_name, img);
        cv::waitKey(1);
        // You can try checking if the window is still visible, and stop if it is not.
        // However, that OpenCV function appears buggy on many systems.
        // If it is disabled, then you will have to CTRL+C in the terminal to kill it
        static constexpr bool stop_on_close = false;
        if (stop_on_close and cv::getWindowProperty(window_name, cv::WND_PROP_VISIBLE) < 1) {
            std::cout << "Stopping..." << std::endl;
            running = false;
        }
    }

private:
    zmq::context_t context;
    zmq::socket_t socket;
    std::string window_name;
};

constexpr auto DEFAULT_IP = "127.0.0.1";
constexpr auto DEFAULT_PORT = "9800";
constexpr auto DEFAULT_TOPIC = nodar::zmq::IMAGE_TOPICS[0];

void printUsage() {
    std::cout << "You should specify the Orin's IP address as well as \n"
                 "the port of the message that you want to listen to like this:\n\n"
                 "     ./image_viewer orin_ip port\n\n"
                 "e.g. ./image_viewer 192.168.1.9 9800\n\n"
                 "Alternatively, you can specify one of the image topic names provided in topic_ports.hpp of zmq_msgs:"
                 "e.g. ./image_viewer 192.168.1.9 nodar/right/image_raw\n\n"
                 "In the meantime, we are going to assume that you are running this on the Orin itself,\n"
                 "and that you want the images on port 9800, that is, we assume that you specified\n\n"
                 "     ./image_viewer 127.0.0.1 9800\n\n"
              << "\n\nNote that the list of topic/port mappings is in topic_ports.hpp header in the zmq_msgs target."
              << "\n----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc < 3) {
        printUsage();
    }
    const auto ip = argc > 1 ? argv[1] : DEFAULT_IP;

    // If no second argument was provided, then assume the default topic.
    // Try to parse the second argument to see if you provided a port number.
    nodar::zmq::Topic topic;
    if (argc < 3) {
        topic = DEFAULT_TOPIC;
    } else {
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
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(topic.port);
    ZMQImageViewer subscriber(endpoint);
    while (running) {
        subscriber.loopOnce();
    }
}
