// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstdint>
#include <string>

namespace drill
{

// model/color_marker.onnx は入力 "mean_bgr" [1,3]（B,G,R を 0..1 にした平均）、
// 出力 "logits" [1,3]（クラス 0=なし, 1=赤, 2=青）です。
// 同点の argmax は添字が小さい方です。
// 画像が空、またはポインタが nullptr のときは -1 を返します。
// セッションは呼び出しの中で作って構いません（ノードにするときはコンストラクタへ上げます）。
int classify_marker_onnx(
  const std::string & model_path,
  const std::uint8_t * bgr,
  int width,
  int height);

}  // namespace drill
