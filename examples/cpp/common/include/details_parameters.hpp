#pragma once

#include <yaml-cpp/yaml.h>

#include <array>
#include <cstdint>
#include <string>

struct DetailsParameters {
    uint64_t leftTime{0};
    uint64_t rightTime{0};
    float focalLength{0.0f};
    float baseline{0.0f};
    float metersAboveGround{0.0f};
    std::array<float, 16> projection{1.0f, 0.0f, 0.0f, 0.0f,  //
                                     0.0f, 1.0f, 0.0f, 0.0f,  //
                                     0.0f, 0.0f, 1.0f, 0.0f,  //
                                     0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 9> rotationDisparityToRawCam{1.0f, 0.0f, 0.0f,  //
                                                   0.0f, 1.0f, 0.0f,  //
                                                   0.0f, 0.0f, 1.0f};
    std::array<float, 9> rotationWorldToRawCam{1.0f, 0.0f, 0.0f,  //
                                               0.0f, 1.0f, 0.0f,  //
                                               0.0f, 0.0f, 1.0f};

    bool parse(const std::string& filePath, bool& hasErrors) {
        const std::string LEFT_TIME{"left_time"};
        const std::string RIGHT_TIME{"right_time"};
        const std::string FOCAL_LENGTH{"focal_length"};
        const std::string BASELINE{"baseline"};
        const std::string METERS_ABOVE_GROUND{"meters_above_ground"};
        const std::string PROJECTION{"projection"};
        const std::string ROTATION_DISPARITY_TO_RAW_CAM{"rotation_disparity_to_raw_cam"};
        const std::string ROTATION_WORLD_TO_RAW_CAM{"rotation_world_to_raw_cam"};

        YAML::Node details{};

        try {
            details = YAML::LoadFile(filePath);
        } catch (...) {
            return false;
        }

        // if even one filed is missing, the whole file is invalid
        // in this case, we should resave it with the default values
        bool noErrors{true};

        noErrors &= read_scalar_field(leftTime, LEFT_TIME, details);
        noErrors &= read_scalar_field(rightTime, RIGHT_TIME, details);
        noErrors &= read_scalar_field(focalLength, FOCAL_LENGTH, details);
        noErrors &= read_scalar_field(baseline, BASELINE, details);
        noErrors &= read_scalar_field(metersAboveGround, METERS_ABOVE_GROUND, details);
        noErrors &= read_collection_field(projection, PROJECTION, details);
        noErrors &= read_collection_field(rotationDisparityToRawCam, ROTATION_DISPARITY_TO_RAW_CAM, details);
        noErrors &= read_collection_field(rotationWorldToRawCam, ROTATION_WORLD_TO_RAW_CAM, details);

        hasErrors = !noErrors;

        return true;
    }

private:
    template <typename T>
    bool read_scalar_field(T& dst, const std::string& fieldName, const YAML::Node& config) {
        try {
            const auto& field{config[fieldName]};
            dst = field.as<T>();
        } catch (...) {
            return false;
        }

        return true;
    }

    template <size_t N>
    bool read_collection_field(std::array<float, N>& dst, const std::string& fieldName, const YAML::Node& config) {
        try {
            const auto& field{config[fieldName]};

            if (field.size() != N) {
                return false;
            }

            for (size_t i{0}; i < N; ++i) {
                dst[i] = field[i].as<float>();
            }
        } catch (...) {
            return false;
        }

        return true;
    }
};
