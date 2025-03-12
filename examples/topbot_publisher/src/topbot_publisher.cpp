#include "topbot_publisher.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "get_files.hpp"
#include "nodar/zmq/topic_ports.hpp"

constexpr auto FRAME_RATE = 10;
constexpr auto DEFAULT_TOPIC = nodar::zmq::EXTERNAL_TOPBOT_TOPICS[0];

std::atomic_bool running{true};

void signalHandler(int) {
    std::cerr << "SIGINT or SIGTERM received. Exiting..." << std::endl;
    running = false;
}

void printUsage() {
    std::cout
        << "You should specify the topbot data directory, as well as \n"
           "the port of the message that you want to publish to like this:\n"
           "     ./topbot_publisher <topbot_data_directory> <port>\n"
           "Alternatively, you can specify one of the external topbot topic names provided in topic_ports.hpp of "
           "zmq_msgs:\n"
           "     ./topbot_publisher <topbot_data_directory> <topic>\n\n"
           "In the meantime, we are going to assume that you are publishing BGR images, that is, we assume "
           "that you specified\n"
           "     ./topbot_publisher <topbot_data_directory> 5000\n"
           "     ./topbot_publisher <topbot_data_directory> external/topbot_bgr\n\n"
        << "Note that the list of EXTERNAL_TOPBOT_TOPICS mappings is in topic_ports.hpp header in the zmq_msgs target."
        << "\n--------------------------------------------------------------------------------------------------\n";
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (argc < 3) {
        printUsage();
    }

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
            for (const auto& external_image_topic : nodar::zmq::EXTERNAL_TOPBOT_TOPICS) {
                if (port == external_image_topic.port) {
                    topic = external_image_topic;
                    invalid_topic = false;
                }
            }
            if (invalid_topic) {
                std::cerr << "It seems like you specified a port number " << port
                          << " that does not correspond to a port on the topic/port mapping list." << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            // It seems like you specified a topic name. Let's see if it is a valid image topic
            const std::string topic_name = argv[2];
            for (const auto& external_image_topic : nodar::zmq::EXTERNAL_TOPBOT_TOPICS) {
                if (topic_name == external_image_topic.name) {
                    topic = external_image_topic;
                    invalid_topic = false;
                }
            }
            if (invalid_topic) {
                std::cerr << "It seems like you specified a topic name " << topic_name
                          << " that does not correspond to a topic on the topic/port mapping list." << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    const auto image_files{getFiles(argv[1], ".tiff")};
    if (image_files.empty()) {
        std::cerr << "No images found in folder." << std::endl;
        return EXIT_FAILURE;
    }

    nodar::zmq::TopbotPublisher publisher(topic);
    auto frame_id = 0;

    while (running) {
        for (const auto& file : image_files) {
            if (!running)
                break;
            const auto img = cv::imread(file, cv::IMREAD_UNCHANGED);
            const auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();
            if (publisher.publishImage(img, timestamp, frame_id)) {
                std::cout << "Published frame " << frame_id << " from " << file << std::endl;
                frame_id++;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FRAME_RATE));
        }
    }

    std::cout << "Publisher stopped." << std::endl;
    return EXIT_SUCCESS;
}
