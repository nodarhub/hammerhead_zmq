#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

// Returns the current UTC time as a compact string (e.g. "20260422-143012")
[[nodiscard]] inline std::string dateString() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm buf{};
#if defined(_WIN32)
    gmtime_s(&buf, &now);
#else
    gmtime_r(&now, &buf);
#endif
    std::ostringstream date_ss;
    date_ss << std::put_time(&buf, "%Y%m%d-%H%M%S");
    return date_ss.str();
}
