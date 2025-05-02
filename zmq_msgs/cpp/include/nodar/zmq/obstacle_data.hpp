#pragma once

#include <array>
#include <nodar/zmq/message_info.hpp>
#include <nodar/zmq/utils.hpp>
#include <type_traits>
#include <vector>

namespace nodar {
namespace zmq {

struct ObstacleData {
    struct Vec2 {
        float x;
        float z;
    };
    struct Vec2int {
        int x;
        int z;
    };
    static_assert(sizeof(Vec2) == 2 * sizeof(float), "The ObstacleData::Vec2 is assumed to be non-padded.");
    static_assert(sizeof(Vec2int) == 2 * sizeof(int), "The ObstacleData::Vec2int is assumed to be non-padded.");

    struct BoundingBox {
        std::array<Vec2, 4> points;
    };
    static_assert(sizeof(BoundingBox) == 8 * sizeof(float),
                  "The ObstacleData::BoundingBox is assumed to be non-padded.");

    struct Obstacle {
        BoundingBox boundingBox;
        Vec2 velocity;
        std::vector<Vec2int> occupancyData;
    };
    static_assert(std::is_trivially_copyable<Obstacle>::value,
                  "The ObstacleData::Obstacle is assumed to be trivially copyable.");

    static constexpr uint64_t HEADER_SIZE{512};
    static constexpr MessageInfo getInfo() { return MessageInfo{8}; }

    // Ideally INFO would be static constexpr, but to handle that nicely we would have to require C++17
    MessageInfo INFO{getInfo()};
    uint64_t time{};
    uint64_t frame_id{};
    uint64_t num_obstacles{0};
    std::vector<Obstacle> obstacles{};

    ObstacleData() = default;

    ObstacleData(uint64_t time_arg, uint64_t frame_id_arg, const std::vector<Obstacle> &obstacles_arg)
        : time{time_arg}, frame_id{frame_id_arg}, num_obstacles{obstacles_arg.size()}, obstacles{obstacles_arg} {}

    explicit ObstacleData(const uint8_t *src) { read(src); }

    [[nodiscard]] static constexpr uint64_t obstacleBytes(uint64_t num_obstacles_arg) {
        return num_obstacles_arg * sizeof(Obstacle);
    }

    [[nodiscard]] static constexpr uint64_t msgSize(uint64_t num_obstacles_arg) {
        return HEADER_SIZE + obstacleBytes(num_obstacles_arg);
    }

    [[nodiscard]] bool empty() const { return num_obstacles == 0; }

    [[nodiscard]] uint64_t obstacleBytes() const { return obstacleBytes(num_obstacles); }

    [[nodiscard]] uint64_t msgSize() const { return msgSize(num_obstacles); }

    void update(uint64_t time_arg, uint64_t frame_id_arg, const std::vector<Obstacle> &obstacles_arg) {
        time = time_arg;
        frame_id = frame_id_arg;
        num_obstacles = obstacles_arg.size();
        obstacles = obstacles_arg;
    }

    void read(const uint8_t *src) {
        const auto mem{src + HEADER_SIZE};

        // Check the info to make sure this message is the type we expect
        MessageInfo info;
        src = utils::read(src, info);
        if (info != INFO) {
            std::cerr << "This message either is not a ObstacleData message, or is a different message version."
                      << std::endl;
            return;
        }

        // Read the basic data types
        src = utils::read(src, time);
        src = utils::read(src, frame_id);
        src = utils::read(src, num_obstacles);

        // Read obstacles
        obstacles.resize(num_obstacles);
        const uint8_t* current_mem = mem;
        for (auto& obstacle : obstacles) {
            // Read bounding box and velocity
            memcpy(&obstacle.boundingBox, current_mem, sizeof(BoundingBox));
            current_mem += sizeof(BoundingBox);
            memcpy(&obstacle.velocity, current_mem, sizeof(Vec2));
            current_mem += sizeof(Vec2);
            
            // Read occupancy data size and data
            uint64_t occupancy_size;
            memcpy(&occupancy_size, current_mem, sizeof(uint64_t));
            current_mem += sizeof(uint64_t);
            obstacle.occupancyData.resize(occupancy_size);
            memcpy(obstacle.occupancyData.data(), current_mem, occupancy_size * sizeof(Vec2int));
            current_mem += occupancy_size * sizeof(Vec2int);
        }
    }

    static auto write(uint8_t *dst, uint64_t time_arg, uint64_t frame_id_arg,
                      const std::vector<Obstacle> &obstacles_arg) {
        auto mem{dst + HEADER_SIZE};
        std::memset(dst, 0, HEADER_SIZE);  // Zero the header before writing

        dst = utils::append(dst, getInfo());
        dst = utils::append(dst, time_arg);
        dst = utils::append(dst, frame_id_arg);
        dst = utils::append(dst, static_cast<uint64_t>(obstacles_arg.size()));

        // Write each obstacle's data
        for (const auto& obstacle : obstacles_arg) {
            // Write bounding box and velocity
            memcpy(mem, &obstacle.boundingBox, sizeof(BoundingBox));
            mem += sizeof(BoundingBox);
            memcpy(mem, &obstacle.velocity, sizeof(Vec2));
            mem += sizeof(Vec2);
            
            // Write occupancy data size and data
            const uint64_t occupancy_size = obstacle.occupancyData.size();
            memcpy(mem, &occupancy_size, sizeof(uint64_t));
            mem += sizeof(uint64_t);
            memcpy(mem, obstacle.occupancyData.data(), occupancy_size * sizeof(Vec2int));
            mem += occupancy_size * sizeof(Vec2int);
        }
        return mem;
    }

    auto write(uint8_t *dst) const { return write(dst, time, frame_id, obstacles); }
};

}  // namespace zmq
}  // namespace nodar
