#pragma once

#include <cstring>
#include <iostream>

#include "message_info.hpp"
#include "utils.hpp"

namespace nodar {
namespace zmq {

struct CameraParameterRequest {
    MessageInfo INFO = MessageInfo(2);
    float val{0};

    CameraParameterRequest() = default;

    explicit CameraParameterRequest(float val_arg) : val(val_arg) {}

    explicit CameraParameterRequest(const uint8_t *src) {
        MessageInfo info;
        src = utils::read(src, info);
        if (info.is_different(INFO, "CameraParameterRequest")) {
            return;
        }
        utils::read(src, val);
    }

    auto write(uint8_t *dst) const {
        dst = utils::append(dst, INFO);
        return utils::append(dst, val);
    }

    [[nodiscard]] static constexpr auto msgSize() { return sizeof(MessageInfo) + sizeof(float); }

    static auto write(uint8_t *dst, float val) { return CameraParameterRequest(val).write(dst); }
};

struct CameraParameterResponse {
    MessageInfo INFO = MessageInfo(3);
    bool success{false};

    CameraParameterResponse() = default;

    explicit CameraParameterResponse(bool success) : success(success) {}

    explicit CameraParameterResponse(const uint8_t *src) {
        MessageInfo info;
        src = utils::read(src, info);
        if (info.is_different(INFO, "CameraParameterResponse")) {
            return;
        }
        utils::read(src, success);
    }

    auto write(uint8_t *dst) const {
        dst = utils::append(dst, INFO);
        return utils::append(dst, success);
    }

    [[nodiscard]] static constexpr auto msgSize() { return sizeof(MessageInfo) + sizeof(bool); }

    static auto write(uint8_t *dst, float val) { return CameraParameterResponse(val).write(dst); }
};

}  // namespace zmq
}  // namespace nodar