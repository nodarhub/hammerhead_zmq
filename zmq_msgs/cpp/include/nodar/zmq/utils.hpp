#pragma once

#include <cstdint>
#include <cstring>

namespace nodar {
namespace zmq {
namespace utils {
template <typename Field>
inline auto append(uint8_t *dst, Field &&field) {
    memcpy(dst, &field, sizeof(field));
    return dst + sizeof(field);
}

template <typename Field>
inline auto read(const uint8_t *dst, Field &field) {
    memcpy(&field, dst, sizeof(field));
    return dst + sizeof(field);
}
}  // namespace utils
}  // namespace zmq
}  // namespace nodar