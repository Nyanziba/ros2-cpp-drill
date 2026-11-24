// I AM NOT DONE
//
// TensorRT のエンジンは GPU が要るので、ここでは精度の選び方、
// フレーム予算、INT8 用の代表画像の選び方を実装します。

#include "drill/precision_budget.hpp"

namespace drill
{

Precision choose_precision(bool gpu_fp16, bool gpu_int8, int calibration_images)
{
  (void)gpu_fp16;
  (void)gpu_int8;
  (void)calibration_images;
  return Precision::Fp32;
}

bool meets_frame_budget(double latency_ms, double camera_fps, double margin)
{
  (void)latency_ms;
  (void)camera_fps;
  (void)margin;
  return false;
}

double speedup_ratio(double baseline_ms, double candidate_ms)
{
  (void)baseline_ms;
  (void)candidate_ms;
  return 0.0;
}

std::vector<std::string> pick_calibration_set(
  const std::vector<HistogramSample> & samples, int count)
{
  (void)samples;
  (void)count;
  return {};
}

bool parse_trtexec_mean_ms(const std::string & line, double & mean_ms)
{
  (void)line;
  (void)mean_ms;
  return false;
}

}  // namespace drill
