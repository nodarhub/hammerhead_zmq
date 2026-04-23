#pragma once

#include <chrono>
#include <deque>
#include <iomanip>
#include <sstream>
#include <string>

// Rolling frame-rate counter. Call tic() once per frame, then str() to get a
// human-readable summary of fps over the last 100 frames and over the full
// run ("fps_100: 30.01, fps_inf: 29.87").
class FPS {
public:
    explicit FPS(size_t maxlen = 100) : frame_times(), start_time(), total_frames(0), maxlen(maxlen) {}

    void tic() {
        auto now = Clock::now();
        if (total_frames == 0) {
            start_time = now;
        }
        frame_times.push_back(now);
        if (frame_times.size() > maxlen) {
            frame_times.pop_front();
        }
        ++total_frames;
    }

    [[nodiscard]] double fps_over_window(size_t n) const {
        if (frame_times.size() < 2) {
            return 0.0;
        }
        if (frame_times.size() < n) {
            n = frame_times.size();
        }
        const auto t1 = frame_times[frame_times.size() - n];
        const auto t2 = frame_times.back();
        const std::chrono::duration<double> delta = t2 - t1;
        return (t2 == t1) ? 0.0 : static_cast<double>(n - 1) / delta.count();
    }

    [[nodiscard]] std::string str() const {
        const double fps_100 = fps_over_window(100);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << "fps_100: " << fps_100 << ", fps_inf: ";
        if (total_frames <= 1) {
            ss << 0.0;
        } else {
            const std::chrono::duration<double> total_time = frame_times.back() - start_time;
            ss << static_cast<double>(total_frames - 1) / total_time.count();
        }
        return ss.str();
    }

private:
    using Clock = std::chrono::steady_clock;
    std::deque<Clock::time_point> frame_times;
    Clock::time_point start_time;
    size_t total_frames;
    size_t maxlen;
};
