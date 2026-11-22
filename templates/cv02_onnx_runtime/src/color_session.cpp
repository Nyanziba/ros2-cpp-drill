// I AM NOT DONE
//
// ONNX Runtime の C++ API で、平均 BGR をモデルに通してクラス番号を返してください。

#include "drill/color_session.hpp"

namespace drill
{

int classify_marker_onnx(
  const std::string & model_path,
  const std::uint8_t * bgr,
  int width,
  int height)
{
  (void)model_path;
  (void)bgr;
  (void)width;
  (void)height;
  // TODO: 画素平均を 255 で割り、入力名 mean_bgr で Run し、logits の argmax を返す。
  return -1;
}

}  // namespace drill
