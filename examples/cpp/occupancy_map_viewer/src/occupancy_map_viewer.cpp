#include <atomic>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <nodar/zmq/image.hpp>
#include <nodar/zmq/opencv_utils.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
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
        std::cout << "==================================== Occupancy Map Metadata ===================================="
                  << std::endl;
        std::cout << "X range: [" << xMin << ", " << xMax << "] meters" << std::endl;
        std::cout << "Z range: [" << zMin << ", " << zMax << "] meters" << std::endl;
        std::cout << "Cell size: " << cellSize << " meters" << std::endl;
        std::cout << "Grid dimensions (X x Z): " << ((xMax - xMin) / cellSize) << " x " << ((zMax - zMin) / cellSize)
                  << " cells" << std::endl;
        std::cout << "================================================================================================"
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

// Get OpenCV type as string
std::string getImageType(int type) {
    std::string r;
    int depth = type & CV_MAT_DEPTH_MASK;
    int chans = 1 + (type >> CV_CN_SHIFT);

    switch (depth) {
        case CV_8U:
            r = "CV_8U";
            break;
        case CV_8S:
            r = "CV_8S";
            break;
        case CV_16U:
            r = "CV_16U";
            break;
        case CV_16S:
            r = "CV_16S";
            break;
        case CV_32S:
            r = "CV_32S";
            break;
        case CV_32F:
            r = "CV_32F";
            break;
        case CV_64F:
            r = "CV_64F";
            break;
        default:
            r = "Unknown";
            break;
    }

    r += "C" + std::to_string(chans);
    return r;
}

// Draw grid overlay with metric labels on the occupancy map
void drawGridOverlay(cv::Mat &display_img, const cv::Mat &occupancy_map, const OccupancyMapMetadata &metadata) {
    const int height = occupancy_map.rows;
    const int width = occupancy_map.cols;

    // Margins for labels
    const int left_margin = 50;
    const int bottom_margin = 50;
    const int top_margin = 20;
    const int right_margin = 20;

    // Create larger image with margins
    const int total_height = top_margin + height + bottom_margin;
    const int total_width = left_margin + width + right_margin;

    // Convert to color and add borders
    cv::Mat color_map;
    cv::cvtColor(occupancy_map, color_map, cv::COLOR_GRAY2BGR);
    cv::copyMakeBorder(color_map, display_img, top_margin, bottom_margin, left_margin, right_margin,
                       cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));  // White margins

    const cv::Scalar grid_color(80, 80, 80);  // Gray for grid lines
    const cv::Scalar x_text_color(0, 0, 255);  // Red for X axis labels
    const cv::Scalar z_text_color(0, 255, 0);  // Green for Z axis labels
    const double font_scale = 0.3;
    const int font_thickness = 1;
    const int line_thickness = 1;

    // Calculate pixels per meter
    const float pixels_per_meter_x = height / (metadata.xMax - metadata.xMin);
    const float pixels_per_meter_z = width / (metadata.zMax - metadata.zMin);

    // Fixed grid spacing of 10 meters
    const float grid_spacing = 10.0f;

    // Draw horizontal grid lines for X values
    float x_start = std::floor(metadata.xMin / grid_spacing) * grid_spacing;
    for (float x = x_start; x <= metadata.xMax; x += grid_spacing) {
        int pixel_x = static_cast<int>((x - metadata.xMin) * pixels_per_meter_x) + top_margin;
        if (pixel_x >= top_margin && pixel_x <= top_margin + height) {
            // Draw grid line across the image area
            cv::line(display_img, cv::Point(left_margin, pixel_x), cv::Point(left_margin + width, pixel_x), grid_color,
                     line_thickness);

            // Draw label in left margin
            std::ostringstream label;
            label << static_cast<int>(x) << "m";
            cv::putText(display_img, label.str(), cv::Point(5, pixel_x + 5), cv::FONT_HERSHEY_SIMPLEX, font_scale,
                        x_text_color, font_thickness);
        }
    }

    // Draw vertical grid lines for Z values
    float z_start = std::floor(metadata.zMin / grid_spacing) * grid_spacing;
    for (float z = z_start; z <= metadata.zMax; z += grid_spacing) {
        int pixel_z = static_cast<int>((z - metadata.zMin) * pixels_per_meter_z) + left_margin;
        if (pixel_z >= left_margin && pixel_z <= left_margin + width) {
            // Draw grid line across the image area
            cv::line(display_img, cv::Point(pixel_z, top_margin), cv::Point(pixel_z, top_margin + height), grid_color,
                     line_thickness);

            // Draw label in bottom margin
            std::ostringstream label;
            label << static_cast<int>(z) << "m";
            cv::putText(display_img, label.str(), cv::Point(pixel_z - 10, total_height - 8), cv::FONT_HERSHEY_SIMPLEX,
                        font_scale, z_text_color, font_thickness);
        }
    }
}

class OccupancyMapViewer {
public:
    uint64_t last_frame_id = 0;

    OccupancyMapViewer(const std::string &endpoint)
        : context(1), socket(context, ZMQ_SUB), window_name("Occupancy Map") {
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
        if (img.empty()) {
            return;
        }

        const auto &frame_id = stamped_image.frame_id;
        if (last_frame_id != 0 and frame_id != last_frame_id + 1) {
            std::cerr << (frame_id - last_frame_id - 1) << " frames dropped. Current frame ID : " << frame_id
                      << ", last frame ID: " << last_frame_id << std::endl;
        }
        last_frame_id = frame_id;

        // Parse metadata from additional_field
        OccupancyMapMetadata metadata;
        bool has_metadata = parseMetadata(stamped_image.additional_field, metadata);
        std::cout << "Frame # " << frame_id << " | Time: " << stamped_image.time << " | Size: " << img.rows << "x"
                  << img.cols << " | Type: " << getImageType(img.type())
                  << " | Occupied cells: " << cv::countNonZero(img) << std::endl;
        if (has_metadata) {
            metadata.print();
        }

        // Create visualization with grid overlay
        cv::Mat display_img;
        drawGridOverlay(display_img, img, metadata);

        // Display the image
        cv::resizeWindow(window_name, {1920, 1080});
        cv::imshow(window_name, display_img);
        cv::waitKey(1);
    }

private:
    zmq::context_t context;
    zmq::socket_t socket;
    std::string window_name;
};

void printUsage(const std::string &default_ip) {
    std::cout << "You should specify the IP address of the device running Hammerhead:\n\n"
                 "     ./occupancy_map_viewer hammerhead_ip\n\n"
                 "e.g. ./occupancy_map_viewer 10.10.1.10\n\n"
                 "In the meantime, we assume that you are running this on the device running Hammerhead,\n"
                 "that is, we assume that you specified\n\n"
                 "     ./occupancy_map_viewer "
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

    OccupancyMapViewer viewer(endpoint);
    while (running) {
        viewer.loopOnce();
    }
}