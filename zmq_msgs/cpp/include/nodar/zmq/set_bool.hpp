#pragma once

#include <cstring>
#include <memory>

#include "message_info.hpp"
#include "utils.hpp"

namespace nodar {
namespace zmq {

struct SetBoolRequest {
    [[nodiscard]] static constexpr auto msgSize() { return sizeof(SetBoolRequest) + sizeof(MessageInfo); }
    [[nodiscard]] static constexpr MessageInfo expected_info() { return MessageInfo(6); }

    bool val{false};

    explicit SetBoolRequest(bool val_arg) : val(val_arg) {}

    static std::unique_ptr<SetBoolRequest> read(const uint8_t *src) {
        MessageInfo info;
        src = utils::read(src, info);
        if (info.is_different(expected_info(), "SetBoolRequest")) {
            return nullptr;
        }
        bool val;
        utils::read(src, val);
        return std::make_unique<SetBoolRequest>(val);
    }

    static auto write(uint8_t *dst, bool val) {
        dst = utils::append(dst, expected_info());
        return utils::append(dst, val);
    }

    auto write(uint8_t *dst) const { return write(dst, val); }
};

struct SetBoolResponse {
    [[nodiscard]] static constexpr auto msgSize() { return sizeof(SetBoolResponse) + sizeof(MessageInfo); }
    [[nodiscard]] static constexpr MessageInfo expected_info() { return MessageInfo(7); }

    bool success{false};

    explicit SetBoolResponse(bool success_arg) : success(success_arg) {}

    static std::unique_ptr<SetBoolResponse> read(const uint8_t *src) {
        MessageInfo info;
        src = utils::read(src, info);
        if (info.is_different(expected_info(), "SetBoolResponse")) {
            return nullptr;
        }
        bool val;
        utils::read(src, val);
        return std::make_unique<SetBoolResponse>(val);
    }

    static auto write(uint8_t *dst, bool val) {
        dst = utils::append(dst, expected_info());
        return utils::append(dst, val);
    }

    auto write(uint8_t *dst) const { return write(dst, success); }
};
}  // namespace zmq
}  // namespace nodar