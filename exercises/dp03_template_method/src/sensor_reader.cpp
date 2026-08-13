// I AM NOT DONE
//
// Template Method を NVI (Non-Virtual Interface) で書きます。
// 骨格は基底クラスが持ち、差し替えたい部分だけを派生クラスが private virtual で埋めます。
//
// include/drill/sensor_reader.hpp は編集しません。宣言はそこにあります。

#include "drill/sensor_reader.hpp"

#include <cmath>
#include <utility>

// ---------------------------------------------------------------------------
// SensorReader — 手順の骨格
// ---------------------------------------------------------------------------

void SensorReader::record(const char * step_name)
{
  call_log_.emplace_back(step_name);
}

bool SensorReader::validate(double converted_value) const
{
  // TODO: 既定の検証。有限な値（NaN でも無限大でもない）なら true を返してください。
  //       <cmath> の std::isfinite が使えます。
  (void)converted_value;
  return false;
}

std::optional<double> SensorReader::read_once()
{
  // TODO: テンプレートメソッド本体を書いてください。手順は次の順番で固定です。
  //
  //   1. まだ初期化していなければ record("initialize") してから initialize() を呼び、
  //      初期化済みにする（2 回目以降は初期化しない）
  //   2. record("fetch_raw") してから fetch_raw() で生値を取る
  //   3. record("convert") してから convert() で物理量に変換する
  //   4. record("validate") してから validate() で検証する
  //   5. 検証に通れば変換後の値を、落ちたら std::nullopt を返す
  //
  // record() は必ず「その段の仮想関数を呼ぶ直前」に呼んでください。
  // テストは call_log() の中身で手順の順番を見ています。
  return std::nullopt;
}

// ---------------------------------------------------------------------------
// EncoderReader — カウント値 → 角度[deg]
// ---------------------------------------------------------------------------

EncoderReader::EncoderReader(std::vector<int> samples, double counts_per_revolution)
: samples_(std::move(samples)), counts_per_revolution_(counts_per_revolution)
{
}

void EncoderReader::initialize()
{
  // TODO: 読み出し位置 next_index_ を 0 に戻してください。
}

int EncoderReader::fetch_raw()
{
  // TODO: samples_ から次の値を 1 つ返し、読み出し位置を 1 進めてください。
  //       もう残っていなければ 0 を返してください（位置は進めなくて構いません）。
  (void)next_index_;
  return 0;
}

double EncoderReader::convert(int raw_value) const
{
  // TODO: カウント値を角度[deg]に変換してください。1 回転 = counts_per_revolution_ カウント。
  (void)raw_value;
  (void)counts_per_revolution_;
  return 0.0;
}

// ---------------------------------------------------------------------------
// ThermistorReader — AD 値 → 温度[degC]
// ---------------------------------------------------------------------------

ThermistorReader::ThermistorReader(std::vector<int> samples)
: samples_(std::move(samples))
{
}

void ThermistorReader::initialize()
{
  // TODO: 読み出し位置 next_index_ を 0 に戻してください。
}

int ThermistorReader::fetch_raw()
{
  // TODO: EncoderReader::fetch_raw() と同じ要領で samples_ から 1 つ返してください。
  (void)next_index_;
  return 0;
}

double ThermistorReader::convert(int raw_value) const
{
  // TODO: AD 値を温度[degC]に変換してください。式は degC = raw_value * 0.1 - 20.0 とします。
  (void)raw_value;
  return 0.0;
}

bool ThermistorReader::validate(double converted_value) const
{
  // TODO: 基底の検証（SensorReader::validate）に通り、かつ
  //       kMinValidCelsius 以上 kMaxValidCelsius 以下なら true を返してください。
  //       基底の実装は SensorReader::validate(converted_value) で呼べます。
  (void)converted_value;
  return false;
}
