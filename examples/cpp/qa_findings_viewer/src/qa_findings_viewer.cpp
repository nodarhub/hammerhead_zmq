#include <atomic>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <nodar/zmq/qa_findings.hpp>
#include <nodar/zmq/topic_ports.hpp>
#include <zmq.hpp>

std::atomic_bool running{true};

void signalHandler(int signum) {
    std::cerr << "SIGINT or SIGTERM received." << std::endl;
    running = false;
}

class QAFindingsViewer {
public:
    uint64_t last_frame_id = 0;

    QAFindingsViewer(const std::string& endpoint) : context(1), socket(context, ZMQ_SUB) {
        const int hwm = 1;  // set maximum queue length to 1 message
        socket.set(zmq::sockopt::rcvhwm, hwm);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(endpoint);
        std::cout << "Subscribing to " << endpoint << std::endl;
    }

    void loopOnce() {
        zmq::message_t msg;
        const auto received_bytes = socket.recv(msg, zmq::recv_flags::none);
        const nodar::zmq::QAFindings qa_msg(static_cast<const uint8_t*>(msg.data()));

        // Warn if we dropped a frame
        const auto& frame_id = qa_msg.frame_id;
        if (last_frame_id != 0 && frame_id != last_frame_id + 1) {
            std::cerr << (frame_id - last_frame_id - 1) << " frames dropped. Current frame ID : " << frame_id
                      << ", last frame ID: " << last_frame_id << std::endl;
        }
        last_frame_id = frame_id;
        std::cout << "\nQA FINDINGS REPORT" << std::endl;
        std::cout << "Time: " << qa_msg.time << " ns" << std::endl;
        std::cout << "Frame ID: " << qa_msg.frame_id << std::endl;
        std::cout << "Total Findings: " << qa_msg.num_findings << std::endl;

        if (qa_msg.num_findings == 0) {
            std::cout << "No findings reported in this message." << std::endl;
        } else {
            std::cout << std::string(60, '-') << std::endl;

            // Group findings by severity
            int info_count = 0, warning_count = 0, error_count = 0;

            for (const auto& finding : qa_msg.findings) {
                switch (finding.severity) {
                    case nodar::zmq::QAFindings::Severity::INFO:
                        info_count++;
                        break;
                    case nodar::zmq::QAFindings::Severity::WARNING:
                        warning_count++;
                        break;
                    case nodar::zmq::QAFindings::Severity::ERROR:
                        error_count++;
                        break;
                }
            }

            std::cout << "Summary: ";
            if (error_count > 0)
                std::cout << error_count << " Error(s) ";
            if (warning_count > 0)
                std::cout << warning_count << " Warning(s) ";
            if (info_count > 0)
                std::cout << info_count << " Info ";
            std::cout << std::endl;

            std::cout << std::string(60, '-') << std::endl;

            // Display findings by severity (errors first)
            for (int severity = 2; severity >= 0; severity--) {
                for (const auto& finding : qa_msg.findings) {
                    if (static_cast<int>(finding.severity) == severity) {
                        displayFinding(finding);
                    }
                }
            }
        }

        std::cout << std::string(80, '=') << std::endl;
    }

    void displayFinding(const nodar::zmq::QAFindings::Finding& finding) {
        std::string severity_text;

        switch (finding.severity) {
            case nodar::zmq::QAFindings::Severity::INFO:
                severity_text = "INFO";
                break;
            case nodar::zmq::QAFindings::Severity::WARNING:
                severity_text = "WARNING";
                break;
            case nodar::zmq::QAFindings::Severity::ERROR:
                severity_text = "ERROR";
                break;
        }

        std::cout << "[" << severity_text << "] " << finding.domain << "::" << finding.key << std::endl;
        std::cout << "   Message: " << finding.message << std::endl;

        if (finding.value != 0.0 || strlen(finding.unit) > 0) {
            std::cout << "   Value: " << std::fixed << std::setprecision(2) << finding.value;
            if (strlen(finding.unit) > 0) {
                std::cout << " " << finding.unit;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    zmq::context_t context;
    zmq::socket_t socket;
};

void printUsage(const std::string& default_ip) {
    std::cout << "You should specify the IP address of the device running hammerhead:\n\n"
                 "     ./qa_findings_viewer hammerhead_ip\n\n"
                 "e.g. ./qa_findings_viewer 10.10.1.10\n\n"
                 "In the meantime, we assume that you are running this on the device running Hammerhead:\n\n"
                 "     ./qa_findings_viewer "
              << default_ip << "\n"
              << "\n----------------------------------------" << std::endl;
}

int main(int argc, char* argv[]) {
    static constexpr auto default_ip = "127.0.0.1";
    static constexpr auto topic = nodar::zmq::QA_FINDINGS_TOPIC;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc == 1) {
        printUsage(default_ip);
    }
    const auto ip = argc > 1 ? argv[1] : default_ip;
    const auto endpoint = std::string("tcp://") + ip + ":" + std::to_string(topic.port);

    QAFindingsViewer viewer(endpoint);
    while (running) {
        viewer.loopOnce();
    }
}