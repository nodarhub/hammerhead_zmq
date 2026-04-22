#pragma once

#include <iomanip>
#include <sstream>
#include <string>

// Returns the frame number as a 9-digit zero-padded string (e.g. "000001234").
[[nodiscard]] inline std::string frameString(uint64_t frame_no) {
    std::ostringstream oss;
    oss << std::setw(9) << std::setfill('0') << frame_no;
    return oss.str();
}
