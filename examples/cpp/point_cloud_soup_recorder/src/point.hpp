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

    PointXYZRGB() = default;
    PointXYZRGB(float x_, float y_, float z_, uint8_t r_, uint8_t g_, uint8_t b_)
        : x(x_), y(y_), z(z_), r(r_), g(g_), b(b_) {}
};
