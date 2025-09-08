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

void printUsage(const std::string &default_ip, const std::string &default_port) {
    std::cout << "You should specify the IP address of the device running hammerhead, \n"
                 "the port of the message that you want to listen to, "
                 "and the folder where you want the data to be saved:\n\n"
                 "     ./image_recorder hammerhead_ip port output_dir\n\n"
                 "e.g. ./image_recorder 10.10.1.10 9800 recorded_images\n\n"
                 "Alternatively, you can specify one of the image topic names in topic_ports.hpp of zmq_msgs:"
                 "e.g. ./image_viewer 10.10.1.10 nodar/right/image_raw\n\n"
                 "In the meantime, we assume that you are running this on the device running Hammerhead,\n"
                 "and that you want the images on port 9800, that is, we assume that you specified\n\n"
                 "     ./image_viewer " +
                     default_ip + " " + default_port + "\n\n"
              << "\n\nNote that the list of topic/port mappings is in topic_ports.hpp header in the zmq_msgs target."
              << "\n----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    constexpr auto default_ip = "127.0.0.1";
    constexpr auto default_topic = nodar::zmq::IMAGE_TOPICS[0];
    const std::string default_port = std::to_string(default_topic.port);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc < 3) {
        printUsage(default_ip, default_port);
    }
    const auto ip = argc > 1 ? argv[1] : default_ip;

    // If no second argument was provided, then assume the default topic.
    // Try to parse the second argument to see if you provided a port number.
    nodar::zmq::Topic topic = default_topic;
    int port = 0;
    bool is_port_number = false;
    if (argc >= 3) {
        // See if we can parse the second argument as a port number
        std::istringstream iss(argv[2]);
        if (iss >> port && port > 0 && port <= 65535) {
            is_port_number = true;
        } else {
            // It seems like you specified a topic name. Let's see if it is a valid image topic
            const std::string topic_name = argv[2];
            bool invalid_topic = true;
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
    std::string endpoint;
    if (is_port_number) {
        endpoint = "tcp://" + std::string(ip) + ":" + std::to_string(port);
    } else {
        endpoint = "tcp://" + std::string(ip) + ":" + std::to_string(topic.port);
    }
    ZMQImageViewer subscriber(endpoint);
    while (running) {
        subscriber.loopOnce();
    }
}
