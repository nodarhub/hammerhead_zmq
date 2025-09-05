#pragma once

#include <array>
#include <cstring>
#include <nodar/zmq/message_info.hpp>
#include <nodar/zmq/utils.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace nodar {
namespace zmq {

struct QAFindings {
    enum class Severity : uint8_t { INFO = 0, WARNING, ERROR };

    struct Finding {
        char domain[32];  // "image", "system", "hammerhead"
        char key[128];  // "temp", etc.
        char message[128];  // "Temperature is too high"
        char unit[16];  // "C"
        double value;  // 75.0
        Severity severity;  // severity level

        // Clean zero-initialization of all char arrays
        Finding() noexcept : domain{}, key{}, message{}, unit{}, value(0.0), severity(Severity::INFO) {}

        static void copy_cstr(char* dst, std::size_t cap, const std::string& src) {
            // Zero-fill the destination and write a NUL-terminated slice
            std::memset(dst, 0, cap);
            // snprintf guarantees NUL-termination within cap
            std::snprintf(dst, cap, "%s", src.c_str());
        }
    };

    static_assert(std::is_trivially_copyable<Finding>::value, "QAFindings::Finding must be trivially copyable.");
    static_assert(sizeof(Finding) == 320, "Finding binary size must remain 320 bytes.");

    static constexpr uint64_t HEADER_SIZE{64};
    static constexpr MessageInfo getInfo() { return MessageInfo{9}; }

    MessageInfo INFO{getInfo()};
    uint64_t time{};
    uint64_t frame_id{};
    uint64_t num_findings{0};
    std::vector<Finding> findings{};

    QAFindings() = default;

    QAFindings(uint64_t time_arg, uint64_t frame_id_arg, const std::vector<Finding>& findings_arg)
        : time{time_arg}, frame_id{frame_id_arg}, num_findings{findings_arg.size()}, findings{findings_arg} {}

    explicit QAFindings(const uint8_t* src) { read(src); }

    [[nodiscard]] static constexpr uint64_t findingsBytes(uint64_t n) { return n * sizeof(Finding); }

    [[nodiscard]] static constexpr uint64_t msgSize(uint64_t n) { return HEADER_SIZE + findingsBytes(n); }

    [[nodiscard]] bool empty() const { return num_findings == 0; }
    [[nodiscard]] uint64_t findingsBytes() const { return findingsBytes(num_findings); }
    [[nodiscard]] uint64_t msgSize() const { return msgSize(num_findings); }

    void update(uint64_t time_arg, uint64_t frame_id_arg, const std::vector<Finding>& findings_arg) {
        time = time_arg;
        frame_id = frame_id_arg;
        num_findings = findings_arg.size();
        findings = findings_arg;
    }

    static auto write(uint8_t* dst, uint64_t time_arg, uint64_t frame_id_arg,
                      const std::vector<Finding>& findings_arg) {
        auto mem{dst + HEADER_SIZE};
        std::memset(dst, 0, HEADER_SIZE);  // Zero the header before writing

        dst = utils::append(dst, getInfo());
        dst = utils::append(dst, time_arg);
        dst = utils::append(dst, frame_id_arg);
        dst = utils::append(dst, static_cast<uint64_t>(findings_arg.size()));

        const auto findings_bytes{findingsBytes(findings_arg.size())};
        std::memcpy(mem, findings_arg.data(), findings_bytes);
        return mem + findings_bytes;
    }

    auto write(uint8_t* dst) const { return write(dst, time, frame_id, findings); }

    void read(const uint8_t* src) {
        const auto mem{src + HEADER_SIZE};

        // Check the info to make sure this message is the type we expect
        MessageInfo info;
        src = utils::read(src, info);
        if (info != INFO) {
            std::cerr << "This message either is not a QAFindings message, or is a different message version.\n";
            return;
        }

        // Read the basic data types
        src = utils::read(src, time);
        src = utils::read(src, frame_id);
        src = utils::read(src, num_findings);

        // Read findings (trivially copyable)
        findings.resize(num_findings);
        std::memcpy(findings.data(), mem, findingsBytes());
    }

    // Helper to convert from nodar::qa::Finding to zmq Finding
    static Finding convertFinding(const std::string& domain, const std::string& key, uint8_t severity,
                                  const std::string& message, double value, const std::string& unit) {
        Finding f;
        Finding::copy_cstr(f.domain, sizeof(f.domain), domain);
        Finding::copy_cstr(f.key, sizeof(f.key), key);
        f.severity = static_cast<Severity>(severity);
        Finding::copy_cstr(f.message, sizeof(f.message), message);
        f.value = value;
        Finding::copy_cstr(f.unit, sizeof(f.unit), unit);
        return f;
    }
};

}  // namespace zmq
}  // namespace nodar
