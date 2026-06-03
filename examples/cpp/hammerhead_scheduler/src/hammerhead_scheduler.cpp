#include <chrono>
#include <iostream>
#include <memory>
#include <nodar/zmq/topic_ports.hpp>
#include <thread>
#include <zmq.hpp>

class Scheduler {
public:
    Scheduler(const std::string &endpoint) : context(1), socket(context, ZMQ_ROUTER) {
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
    }

    void loop() {
        // Wait for a scheduler request from hammerhead indicating that a frame is ready.
        zmq::message_t identity_msg;
        const auto identity = socket.recv(identity_msg, zmq::recv_flags::none);
        if (not identity) {
            std::cerr << "There was an error waiting for the scheduler request's identity message" << std::endl;
            return;
        }
        ::zmq::message_t request_msg;
        const auto request = socket.recv(request_msg);
        if (not request) {
            std::cerr << "There was an error waiting for the scheduler request" << std::endl;
            return;
        }
        if (request_msg.size() != sizeof(uint64_t)) {
            std::cerr << "The scheduler request doesn't seem to be the right size." << std::endl;
            return;
        }

        // Deserialize the request to get the frame id
        uint64_t frame_id;
        std::memcpy(&frame_id, request_msg.data(), sizeof(frame_id));
        std::cout << "\rGot a scheduler request for frame " << frame_id << std::flush;
        if (initialized and frame_id != last_frame_id + 1) {
            std::cerr << "\nIt looks like we might be dropping frames\n"
                      << "Current frame_id = " << frame_id << "\n"
                      << "Last    frame_id = " << last_frame_id << std::endl;
        }
        initialized = true;
        last_frame_id = frame_id;

        // Halt hammerhead execution before telling it to continue
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        zmq::message_t reply(0);  // Intentionally empty message
        socket.send(identity_msg, zmq::send_flags::sndmore);
        socket.send(reply, zmq::send_flags::none);
    }

private:
    zmq::context_t context;
    zmq::socket_t socket;
    uint64_t last_frame_id{0};
    bool initialized = false;
};

constexpr auto DEFAULT_IP = "127.0.0.1";

void printUsage() {
    std::cout << "You should specify the IP address of the device running hammerhead:\n\n"
                 "     ./hammerhead_scheduler IP\n\n"
                 "e.g. ./hammerhead_scheduler 10.10.1.10\n\n"
                 "In the meantime, we assume that you are running this on the device running Hammerhead,\n"
                 "that is, we assume that you specified "
              << DEFAULT_IP << "\n----------------------------------------" << std::endl;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printUsage();
    }
    const auto ip = argc > 1 ? argv[1] : DEFAULT_IP;
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(nodar::zmq::WAIT_TOPIC.port);
    Scheduler scheduler(endpoint);

    // Infinite loop that runs
    while (true) {
        scheduler.loop();
    }
    return 0;
}