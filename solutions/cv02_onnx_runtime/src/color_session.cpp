#include "drill/color_session.hpp"

#include <array>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace drill
{

int classify_marker_onnx(
  const std::string & model_path,
  const std::uint8_t * bgr,
  int width,
  int height)
{
  if (bgr == nullptr || width <= 0 || height <= 0) {
    return -1;
  }

  const int pixels = width * height;
  double sum_b = 0.0;
  double sum_g = 0.0;
  double sum_r = 0.0;
  for (int i = 0; i < pixels; ++i) {
    sum_b += bgr[i * 3];
    sum_g += bgr[i * 3 + 1];
    sum_r += bgr[i * 3 + 2];
  }
  const float scale = static_cast<float>(pixels * 255.0);
  std::array<float, 3> mean{
    static_cast<float>(sum_b) / scale,
    static_cast<float>(sum_g) / scale,
    static_cast<float>(sum_r) / scale};

  Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "drill_cv02");
  Ort::SessionOptions options;
  options.SetIntraOpNumThreads(1);
  Ort::Session session(env, model_path.c_str(), options);

  std::array<int64_t, 2> shape{1, 3};
  Ort::MemoryInfo memory = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
  Ort::Value input = Ort::Value::CreateTensor<float>(
    memory, mean.data(), mean.size(), shape.data(), shape.size());

  const char * input_names[] = {"mean_bgr"};
  const char * output_names[] = {"logits"};
  auto outputs = session.Run(
    Ort::RunOptions{nullptr}, input_names, &input, 1, output_names, 1);
  const float * logits = outputs.at(0).GetTensorData<float>();

  int best = 0;
  for (int i = 1; i < 3; ++i) {
    if (logits[i] > logits[best]) {
      best = i;
    }
  }
  return best;
}

}  // namespace drill
