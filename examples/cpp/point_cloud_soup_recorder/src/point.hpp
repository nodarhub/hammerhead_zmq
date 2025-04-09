#pragma once

#include <cstdint>

struct PointXYZRGB {
    float x;
    float y;
    float z;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a = 1;
};