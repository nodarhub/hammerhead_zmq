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
    uint64_t last_frame_id{0};

    ObstacleDataSink(const std::filesystem::path &output_dir, const std::string &endpoint)
        : output_dir{output_dir}, context{1}, socket{context, ZMQ_SUB} {
        const int hwm{1};  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
    }

    void loopOnce(size_t frame_index) {
        zmq::message_t msg;
        const auto received_bytes{socket.recv(msg, zmq::recv_flags::none)};
        const nodar::zmq::ObstacleData obstacleData{static_cast<uint8_t *>(msg.data())};
        const auto frame_id{obstacleData.frame_id};
        std::cout << "\rFrame # " << frame_id << ". " << std::flush;

        // If the obstacle data was not received correctly, return
        if (obstacleData.obstacles.empty()) {
            return;
        }

        // Warn if we dropped a frame
        if ((last_frame_id != 0) and (frame_id != last_frame_id + 1)) {
            std::cerr << (frame_id - last_frame_id - 1) << " frames dropped. Current frame ID : " << frame_id
                      << ", last frame ID: " << last_frame_id << std::endl;
        }
        last_frame_id = frame_id;

        // Note that this frame index represents the number of frames received.
        // The messages themselves have a frame_id that tracks how many frames have been produced.
        // Due to networking issues, it could be that there are dropped frames, resulting in these numbers being
        // different
        const auto filename{output_dir / (std::to_string(frame_index) + ".txt")};
        std::cout << "Writing " << filename << std::endl;
        write_data(filename, obstacleData);
    }

private:
    std::filesystem::path output_dir;
    zmq::context_t context;
    zmq::socket_t socket;
};

constexpr auto DEFAULT_IP = "127.0.0.1";

void printUsage() {
    std::cout << "You should specify the Orin's IP address:\n\n"
                 "     ./obstacle_data_generator orin_ip\n\n"
                 "e.g. ./obstacle_data_generator 192.168.1.9\n\n"
                 "In the meantime, we are going to assume that you are running this on the Orin itself,\n"
                 "that is, we assume that you specified\n\n     ./obstacle_data_generator "
              << DEFAULT_IP << "\n----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    constexpr auto TOPIC{nodar::zmq::OBSTACLE_TOPIC};
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc == 1) {
        printUsage();
    }
    const auto ip{argc > 1 ? argv[1] : DEFAULT_IP};
    const auto endpoint{std::string("tcp://") + ip + ":" + std::to_string(TOPIC.port)};

    const auto HERE{std::filesystem::path(__FILE__).parent_path()};
    const auto output_dir{HERE / "obstacle_datas"};
    std::filesystem::create_directories(output_dir);

    ObstacleDataSink sink{output_dir, endpoint};
    size_t frame_index{0};
    while (running) {
        // Note that this frame index represents the number of frames received.
        // The messages themselves have a frame_id that tracks how many frames have been produced.
        // Due to networking issues, it could be that there are dropped frames, resulting in these numbers being
        // different
        sink.loopOnce(frame_index++);
    }
}