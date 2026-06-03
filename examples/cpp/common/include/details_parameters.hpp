#pragma once

#include <yaml-cpp/yaml.h>

#include <array>
#include <fstream>
#include <string>

struct DetailsParameters {
    uint64_t leftTime{0};
    uint64_t rightTime{0};
    float exposure{0.0f};
    float gain{0.0f};
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

    bool save(const std::string& filePath) const {
        const std::string LEFT_TIME{"left_time"};
        const std::string RIGHT_TIME{"right_time"};
        const std::string EXPOSURE{"exposure"};
        const std::string GAIN{"gain"};
        const std::string FOCAL_LENGTH{"focal_length"};
        const std::string BASELINE{"baseline"};
        const std::string METERS_ABOVE_GROUND{"meters_above_ground"};
        const std::string PROJECTION{"projection"};
        const std::string ROTATION_DISPARITY_TO_RAW_CAM{"rotation_disparity_to_raw_cam"};
        const std::string ROTATION_WORLD_TO_RAW_CAM{"rotation_world_to_raw_cam"};

        YAML::Node details{};
        details[LEFT_TIME] = leftTime;
        details[RIGHT_TIME] = rightTime;
        details[EXPOSURE] = exposure;
        details[GAIN] = gain;
        details[FOCAL_LENGTH] = focalLength;
        details[BASELINE] = baseline;
        details[METERS_ABOVE_GROUND] = metersAboveGround;
        details[PROJECTION] = std::vector<float>(projection.begin(), projection.end());
        details[ROTATION_DISPARITY_TO_RAW_CAM] =
            std::vector<float>(rotationDisparityToRawCam.begin(), rotationDisparityToRawCam.end());
        details[ROTATION_WORLD_TO_RAW_CAM] =
            std::vector<float>(rotationWorldToRawCam.begin(), rotationWorldToRawCam.end());

        std::ofstream out(filePath);
        if (!out) {
            return false;
        }
        out << details;
        return static_cast<bool>(out);
    }

    bool parse(const std::string& filePath, bool& hasErrors) {
        const std::string LEFT_TIME{"left_time"};
        const std::string RIGHT_TIME{"right_time"};
        const std::string EXPOSURE{"exposure"};
        const std::string GAIN{"gain"};
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

        // All fields are optional — missing ones keep their default values.
        bool noErrors{true};

        noErrors &= read_scalar_field(leftTime, LEFT_TIME, details, true);
        noErrors &= read_scalar_field(rightTime, RIGHT_TIME, details, true);
        noErrors &= read_scalar_field(exposure, EXPOSURE, details, true);
        noErrors &= read_scalar_field(gain, GAIN, details, true);
        noErrors &= read_scalar_field(focalLength, FOCAL_LENGTH, details, true);
        noErrors &= read_scalar_field(baseline, BASELINE, details, true);
        noErrors &= read_scalar_field(metersAboveGround, METERS_ABOVE_GROUND, details, true);
        noErrors &= read_collection_field(projection, PROJECTION, details, true);
        noErrors &= read_collection_field(rotationDisparityToRawCam, ROTATION_DISPARITY_TO_RAW_CAM, details, true);
        noErrors &= read_collection_field(rotationWorldToRawCam, ROTATION_WORLD_TO_RAW_CAM, details, true);

        hasErrors = !noErrors;

        return true;
    }

private:
    template <typename T>
    bool read_scalar_field(T& dst, const std::string& fieldName, const YAML::Node& config, bool optional = false) {
        const auto& field{config[fieldName]};
        if (!field) {
            return optional;
        }
        try {
            dst = field.as<T>();
        } catch (...) {
            return false;
        }
        return true;
    }

    template <size_t N>
    bool read_collection_field(std::array<float, N>& dst, const std::string& fieldName, const YAML::Node& config,
                               bool optional = false) {
        const auto& field{config[fieldName]};
        if (!field) {
            return optional;
        }
        try {
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
