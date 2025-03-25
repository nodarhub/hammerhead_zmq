#pragma once

// Clang-tidy warns to remove this, but I believe MSVC requires it
#include <iso646.h>

#include <cstdint>
#include <iostream>

namespace nodar {
namespace zmq {

struct MessageInfo {
    static constexpr uint8_t MAJOR_VERSION = 0;
    static constexpr uint8_t MINOR_VERSION = 1;

    uint16_t message_type = 0;
    uint8_t major_version = MAJOR_VERSION;
    uint8_t minor_version = MINOR_VERSION;

    constexpr MessageInfo() = default;

    constexpr explicit MessageInfo(uint16_t message_type_arg) : message_type(message_type_arg) {}

    constexpr bool operator==(const MessageInfo &rhs) const {
        return message_type == rhs.message_type and  //
               major_version == rhs.major_version and  //
               minor_version == rhs.minor_version;
    }

    constexpr bool operator!=(const MessageInfo &rhs) const { return not(*this == rhs); }

    /**
     * This is an inequality check that prints out some extra error information
     */
    constexpr bool is_different(const MessageInfo &rhs, const char *expected_type) const {
        if (message_type != rhs.message_type) {
            std::cerr << "This message is not a " << expected_type << " message." << std::endl;
            return true;
        }
        if (major_version != rhs.major_version) {
            std::cerr << expected_type << " message major versions differ: " << major_version
                      << " != " << rhs.major_version << std::endl;
            return true;
        }
        if (minor_version != rhs.minor_version) {
            std::cerr << expected_type << " message minor versions differ: " << minor_version
                      << " != " << rhs.minor_version << std::endl;
            return true;
        }
        return false;
    }
};

}  // namespace zmq
}  // namespace nodar