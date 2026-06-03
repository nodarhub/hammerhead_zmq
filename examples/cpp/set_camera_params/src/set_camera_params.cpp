#include <iostream>
#include <memory>
#include <nodar/zmq/camera_param.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <zmq.hpp>

#ifdef TOPIC_exposure
static constexpr auto TOPIC = nodar::zmq::CAMERA_EXPOSURE_TOPIC;
#elif TOPIC_gain
static constexpr auto TOPIC = nodar::zmq::CAMERA_GAIN_TOPIC;
#endif

class ClientNode {
public:
    ClientNode(const std::string &endpoint) : context(1), socket(context, ZMQ_REQ) {
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
    }

    void sendRequest(float val) {
        nodar::zmq::CameraParameterRequest request{val};
        ::zmq::message_t request_msg(request.msgSize());
        request.write(static_cast<uint8_t *>(request_msg.data()));
        socket.send(request_msg, ::zmq::send_flags::none);

        ::zmq::message_t response_msg;
        const auto received_bytes = socket.recv(response_msg, ::zmq::recv_flags::none);
        const nodar::zmq::CameraParameterResponse response(static_cast<uint8_t *>(response_msg.data()));
        std::cout << "Client" << std::endl;
        std::cout << "    request->val      : " << request.val << std::endl;
        std::cout << "    response->success : " << response.success << std::endl;
    }

private:
    zmq::context_t context;
    zmq::socket_t socket;
};

constexpr auto DEFAULT_IP = "127.0.0.1";

void printUsage() {
    std::cout << "You should specify the IP address of the device running hammerhead:\n\n"
#ifdef TOPIC_exposure
                 "     ./set_exposure hammerhead_ip\n\n"
                 "e.g. ./set_exposure 10.10.1.10\n\n"
#else
                 "     ./set_gain hammerhead_ip\n\n"
                 "e.g. ./set_gain 10.10.1.10\n\n"
#endif
                 "In the meantime, we assume that you are running this on the device running Hammerhead,\n"
                 "that is, we assume that you specified "
              << DEFAULT_IP << "\n----------------------------------------" << std::endl;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printUsage();
    }
    std::cout << "\n\n--------------------\n"
              << TOPIC.name << "\nTo set a parameter, just input the desired value, and press ENTER.\n"
              << "--------------------\n";
    const auto ip = argc > 1 ? argv[1] : DEFAULT_IP;
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(TOPIC.port);
    ClientNode client_node(endpoint);

    // Infinite loop that runs
    while (true) {
        int val;
        if (std::cin >> val) {
            std::cout << "Requesting " << TOPIC.name << " = " << val << std::endl;
            client_node.sendRequest(val);
        } else {
            std::cerr << "Unknown input. Exiting..." << std::endl;
        }
    }
    return 0;
}