// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <array>
#include <string>
#include <vector>

namespace drill
{

enum class Precision
{
  Fp32,
  Fp16,
  Int8,
};

// INT8 はキャリブレーション画像が 32 枚以上あるときだけ。
// 足りなければ FP16（GPU が対応しているとき）、それも無ければ FP32。
Precision choose_precision(bool gpu_fp16, bool gpu_int8, int calibration_images);

// latency_ms <= (1000 / camera_fps) * (1 - margin) なら true。
// camera_fps が 0 以下なら false。
bool meets_frame_budget(double latency_ms, double camera_fps, double margin);

// baseline_ms / candidate_ms。candidate_ms が 0 以下なら 0。
double speedup_ratio(double baseline_ms, double candidate_ms);

struct HistogramSample
{
  std::string name;
  std::array<int, 8> bins{};
};

// 先頭を種に、L1 距離が最も遠いサンプルを count 枚になるまで足します。
// 距離が同じときは添字が小さい方です。count <= 0 または samples が空なら空。
std::vector<std::string> pick_calibration_set(
  const std::vector<HistogramSample> & samples, int count);

// "mean = 1.80 ms" の形から mean のミリ秒を読む。無ければ false。
bool parse_trtexec_mean_ms(const std::string & line, double & mean_ms);

}  // namespace drill
