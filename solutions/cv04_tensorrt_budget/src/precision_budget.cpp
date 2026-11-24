#include "drill/precision_budget.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace drill
{
namespace
{

int l1(const std::array<int, 8> & left, const std::array<int, 8> & right)
{
  int distance = 0;
  for (int i = 0; i < 8; ++i) {
    distance += std::abs(left[static_cast<std::size_t>(i)] - right[static_cast<std::size_t>(i)]);
  }
  return distance;
}

}  // namespace

Precision choose_precision(bool gpu_fp16, bool gpu_int8, int calibration_images)
{
  if (gpu_int8 && calibration_images >= 32) {
    return Precision::Int8;
  }
  if (gpu_fp16) {
    return Precision::Fp16;
  }
  return Precision::Fp32;
}

bool meets_frame_budget(double latency_ms, double camera_fps, double margin)
{
  if (camera_fps <= 0.0) {
    return false;
  }
  const double limit = (1000.0 / camera_fps) * (1.0 - margin);
  return latency_ms <= limit;
}

double speedup_ratio(double baseline_ms, double candidate_ms)
{
  if (candidate_ms <= 0.0) {
    return 0.0;
  }
  return baseline_ms / candidate_ms;
}

std::vector<std::string> pick_calibration_set(
  const std::vector<HistogramSample> & samples, int count)
{
  std::vector<std::string> chosen;
  if (count <= 0 || samples.empty()) {
    return chosen;
  }
  chosen.push_back(samples.front().name);
  std::vector<int> chosen_index{0};
  while (static_cast<int>(chosen.size()) < count &&
    static_cast<int>(chosen.size()) < static_cast<int>(samples.size()))
  {
    int best_index = -1;
    int best_distance = -1;
    for (int index = 0; index < static_cast<int>(samples.size()); ++index) {
      bool already = false;
      for (int picked : chosen_index) {
        if (picked == index) {
          already = true;
          break;
        }
      }
      if (already) {
        continue;
      }
      int nearest = std::numeric_limits<int>::max();
      for (int picked : chosen_index) {
        nearest = std::min(nearest, l1(samples[static_cast<std::size_t>(index)].bins,
          samples[static_cast<std::size_t>(picked)].bins));
      }
      if (nearest > best_distance) {
        best_distance = nearest;
        best_index = index;
      }
    }
    if (best_index < 0) {
      break;
    }
    chosen_index.push_back(best_index);
    chosen.push_back(samples[static_cast<std::size_t>(best_index)].name);
  }
  return chosen;
}

bool parse_trtexec_mean_ms(const std::string & line, double & mean_ms)
{
  const std::string key = "mean = ";
  const std::size_t pos = line.find(key);
  if (pos == std::string::npos) {
    return false;
  }
  const char * start = line.c_str() + pos + key.size();
  char * end = nullptr;
  const double value = std::strtod(start, &end);
  if (end == start) {
    return false;
  }
  mean_ms = value;
  return true;
}

}  // namespace drill
