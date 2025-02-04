#pragma once

#include <fstream>
#include <sstream>

#include <opencv2/core/mat.hpp>

struct Details {

  template <typename String>
  Details(const String &filename) : projection(4, 4, CV_32FC1) {
    std::ifstream details_file(filename);
    std::string header, detail_data;
    std::getline(details_file, header);
    std::getline(details_file, detail_data);
    std::string token;
    std::istringstream details(detail_data);
    for (auto val : {&left_time, &right_time, &focal_length, &baseline}) {
      std::getline(details, token, ',');
      std::istringstream ss(token);
      ss >> *val;
    }
    for (size_t i = 0; i < projection.total(); ++i) {
      std::getline(details, token, ',');
      std::istringstream ss(token);
      ss >> projection.at<float>(i);
    }
  }

  float left_time;
  float right_time;
  float focal_length;
  float baseline;
  cv::Mat projection;
};