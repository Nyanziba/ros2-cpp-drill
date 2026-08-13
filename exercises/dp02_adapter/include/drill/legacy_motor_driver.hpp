// このファイルは編集しません（インタフェースの提示）。
//
// 3 年前に先輩が書いた、部内で使い回されている「生ドライバ」です。
// 実機で動いている実績があり、命名も単位も今の流儀とは違いますが、
// **書き換えられません**（他の 5 つのプロジェクトが依存しているため）。
//
// - 仮想関数がない。仮想デストラクタもない
// - 命名が camelCase
// - 単位が生値（パルス、エンコーダカウント）
//
// これを、部内共通インタフェース MotorActuator に合わせるのが Adapter の仕事です。
#pragma once

#include <cstdint>

/// 既存の生モータドライバ（変更不可）。
class LegacyMotorDriver
{
public:
  /// このドライバが受け付けるパルス指令の上限（絶対値）。
  static constexpr int MAX_PULSE = 1000;

  /// パルス指令を与える。範囲外は [-MAX_PULSE, MAX_PULSE] に丸められる。
  void setPulse(int pulse)
  {
    if (pulse > MAX_PULSE) {
      pulse = MAX_PULSE;
    }
    if (pulse < -MAX_PULSE) {
      pulse = -MAX_PULSE;
    }
    pulse_ = pulse;
  }

  /// 現在のパルス指令を返す。
  int getPulse() const { return pulse_; }

  /// 全チャンネルを停止する（パルスを 0 にする）。
  void stopAll() { pulse_ = 0; }

  /// エンコーダの生カウントを返す。
  std::int32_t readEncoderRaw() const { return encoder_raw_; }

  /// テスト用。エンコーダの生カウントを外から差し込む。
  void injectEncoderRaw(std::int32_t raw) { encoder_raw_ = raw; }

private:
  int pulse_ = 0;
  std::int32_t encoder_raw_ = 0;
};
