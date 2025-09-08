#include <atomic>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nodar/zmq/obstacle_data.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <zmq.hpp>

std::atomic_bool running{true};

void signalHandler(int signum) {
    std::cerr << "SIGINT or SIGTERM received." << std::endl;
    running = false;
}

inline void write_data(const std::string &filename, const nodar::zmq::ObstacleData &obstacleData) {
    std::ofstream out{filename};
    out << "x1," << "z1," << "x2," << "z2," << "x3," << "z3," << "x4," << "z4," << "vx," << "vz" << "\n";
    for (const auto &obstacle : obstacleData.obstacles) {
        for (const auto &p : obstacle.boundingBox.points) {
            out << std::fixed << std::setprecision(6) << p.x << "," << p.z << ",";
        }
        out << obstacle.velocity.x << "," << obstacle.velocity.z << '\n';
    }
    out << std::endl;
}

class ObstacleDataSink {
public:
    uint64_t last_frame_id = 0;

    ObstacleDataSink(const std::filesystem::path &output_dir, const std::string &endpoint)
        : output_dir(output_dir), context(1), socket(context, ZMQ_SUB) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
    }

    void loopOnce() {
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        const nodar::zmq::ObstacleData obstacleData{static_cast<uint8_t *>(msg.data())};
        const auto &frame_id = obstacleData.frame_id;

        // Warn if we dropped a frame
        if ((last_frame_id != 0) and (frame_id != last_frame_id + 1)) {
            std::cerr << (frame_id - last_frame_id - 1) << " frames dropped. Current frame ID : " << frame_id
                      << ", last frame ID: " << last_frame_id << std::endl;
        }
        last_frame_id = frame_id;
        std::cout << "\rFrame # " << frame_id << ". " << std::flush;

        std::ostringstream filename_ss;
        filename_ss << std::setw(9) << std::setfill('0') << frame_id << ".txt";
        const auto filename = output_dir / filename_ss.str();
        std::cout << "Writing " << filename << std::flush;
        write_data(filename, obstacleData);
    }

private:
    std::filesystem::path output_dir;
    zmq::context_t context;
    zmq::socket_t socket;
};

void printUsage(const std::string &default_ip) {
    std::cout << "You should specify the IP address of the device running hammerhead:\n\n"
                 "     ./obstacle_data_recorder hammerhead_ip\n\n"
                 "e.g. ./obstacle_data_recorder 10.10.1.10\n\n"
                 "In the meantime, we assume that you are running this on the device running Hammerhead,\n"
                 "that is, we assume that you specified\n\n"
                 "     ./obstacle_data_recorder "
              << default_ip << "\n----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    static constexpr auto default_ip = "127.0.0.1";
    static constexpr auto topic = nodar::zmq::OBSTACLE_TOPIC;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc == 1) {
        printUsage(default_ip);
    }
    const auto ip{argc > 1 ? argv[1] : default_ip};
    const auto endpoint{std::string("tcp://") + ip + ":" + std::to_string(topic.port)};

    const auto HERE = std::filesystem::path(__FILE__).parent_path();
    const auto output_dir = HERE / "obstacle_datas";
    std::filesystem::create_directories(output_dir);

    ObstacleDataSink sink(output_dir, endpoint);
    while (running) {
        sink.loopOnce();
    }
}