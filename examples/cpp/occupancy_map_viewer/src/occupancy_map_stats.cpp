#include <atomic>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <zmq.hpp>

std::atomic_bool running{true};

void signalHandler(int signum) {
    std::cerr << "SIGINT or SIGTERM received." << std::endl;
    running = false;
}

struct OccupancyMapMetadata {
    float xMin;
    float xMax;
    float zMin;
    float zMax;
    float cellSize;

    void print() const {
        std::cout << "============================================== Occupancy Map Metadata "
                     "=============================================="
                  << std::endl;
        std::cout << "X range: [" << xMin << ", " << xMax << "] meters" << std::endl;
        std::cout << "Z range: [" << zMin << ", " << zMax << "] meters" << std::endl;
        std::cout << "Cell size: " << cellSize << " meters" << std::endl;
        std::cout << "Grid dimensions (X x Z): " << ((xMax - xMin) / cellSize) << " x " << ((zMax - zMin) / cellSize)
                  << " cells" << std::endl;
        std::cout << "================================================================================================="
                     "==================="
                  << std::endl;
    }
};

bool parseMetadata(const std::vector<uint8_t> &additional_field, OccupancyMapMetadata &metadata) {
    constexpr size_t expected_size = 5 * sizeof(float);  // 5 floats = 20 bytes
    if (additional_field.size() != expected_size) {
        std::cerr << "Warning: Expected " << expected_size << " bytes of metadata, got " << additional_field.size()
                  << " bytes" << std::endl;
        return false;
    }

    const auto *data = additional_field.data();
    memcpy(&metadata.xMin, data, sizeof(float));
    data += sizeof(float);
    memcpy(&metadata.xMax, data, sizeof(float));
    data += sizeof(float);
    memcpy(&metadata.zMin, data, sizeof(float));
    data += sizeof(float);
    memcpy(&metadata.zMax, data, sizeof(float));
    data += sizeof(float);
    memcpy(&metadata.cellSize, data, sizeof(float));

    return true;
}

// Count non-zero bytes in the image data (occupied cells)
uint32_t countOccupiedCells(const std::vector<uint8_t> &img_data) {
    uint32_t count = 0;
    for (const auto &pixel : img_data) {
        if (pixel != 0) {
            count++;
        }
    }
    return count;
}

// Get image type description (compatible with OpenCV type codes)
std::string getImageType(uint32_t type) {
    std::string r;

    // Extract depth (lower 3 bits)
    int depth = type & 0x7;
    // Extract channels (upper bits)
    int chans = 1 + (type >> 3);

    switch (depth) {
        case 0:
            r = "CV_8U";
            break;
        case 1:
            r = "CV_8S";
            break;
        case 2:
            r = "CV_16U";
            break;
        case 3:
            r = "CV_16S";
            break;
        case 4:
            r = "CV_32S";
            break;
        case 5:
            r = "CV_32F";
            break;
        case 6:
            r = "CV_64F";
            break;
        default:
            r = "Unknown";
            break;
    }

    r += "C" + std::to_string(chans);
    return r;
}

class OccupancyMapStats {
public:
    uint64_t last_frame_id = 0;

    OccupancyMapStats(const std::string &endpoint) : context(1), socket(context, ZMQ_SUB) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
    }

    void loopOnce() {
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        const nodar::zmq::StampedImage stamped_image(static_cast<uint8_t *>(msg.data()));

        if (stamped_image.empty()) {
            return;
        }

        const auto &frame_id = stamped_image.frame_id;
        if (last_frame_id != 0 && frame_id != last_frame_id + 1) {
            std::cerr << (frame_id - last_frame_id - 1) << " frames dropped. Current frame ID : " << frame_id
                      << ", last frame ID: " << last_frame_id << std::endl;
        }
        last_frame_id = frame_id;

        // Parse metadata from additional_field
        OccupancyMapMetadata metadata;
        bool has_metadata = parseMetadata(stamped_image.additional_field, metadata);

        // Count occupied cells
        uint32_t occupied_cells = countOccupiedCells(stamped_image.img);
        uint32_t total_cells = stamped_image.rows * stamped_image.cols;
        float occupancy_percentage = (total_cells > 0) ? (100.0f * occupied_cells / total_cells) : 0.0f;

        // Print frame information
        std::cout << "Frame # " << frame_id << " | Time: " << stamped_image.time << " | Size: " << stamped_image.rows
                  << "x" << stamped_image.cols << " | Type: " << getImageType(stamped_image.type)
                  << " | Occupied cells: " << occupied_cells << " / " << total_cells << " (" << std::fixed
                  << std::setprecision(2) << occupancy_percentage << "%)" << std::endl;

        if (has_metadata) {
            metadata.print();
        }
    }

private:
    zmq::context_t context;
    zmq::socket_t socket;
};

void printUsage(const std::string &default_ip) {
    std::cout << "You should specify the IP address of the device running Hammerhead:\n\n"
                 "     ./occupancy_map_stats hammerhead_ip\n\n"
                 "e.g. ./occupancy_map_stats 10.10.1.10\n\n"
                 "In the meantime, we assume that you are running this on the device running Hammerhead,\n"
                 "that is, we assume that you specified\n\n"
                 "     ./occupancy_map_stats "
              << default_ip << "\n\n"
              << "Note: Make sure 'enable_grid_detect = 1' in master_config.ini\n"
              << "----------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    static constexpr auto default_ip = "127.0.0.1";
    static constexpr uint16_t occupancy_map_port = 9900;  // From private_topic_ports.hpp
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc == 1) {
        printUsage(default_ip);
    }
    const auto ip{argc > 1 ? argv[1] : default_ip};
    const auto endpoint{std::string("tcp://") + ip + ":" + std::to_string(occupancy_map_port)};

    OccupancyMapStats viewer(endpoint);
    while (running) {
        viewer.loopOnce();
    }
}