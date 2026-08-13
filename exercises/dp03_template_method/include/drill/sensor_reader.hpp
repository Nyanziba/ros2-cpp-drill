// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

/// センサ読み取りの「決まった手順」を持つ基底クラス。
///
/// NVI (Non-Virtual Interface) イディオムで書かれています。
///   - 手順の骨格 read_once() は public だが virtual ではない（＝差し替え禁止）
///   - 手順の各段は private virtual（＝派生が差し替える）
/// C++ では private な仮想関数も派生クラスからオーバーライドできます。
/// 「呼べるか」と「オーバーライドできるか」は別の話です。
class SensorReader
{
public:
  virtual ~SensorReader() = default;

  SensorReader(const SensorReader &) = delete;
  SensorReader & operator=(const SensorReader &) = delete;

  /// テンプレートメソッド（手順の骨格）。
  /// 初期化（初回のみ）→ 生値の取得 → 物理量への変換 → 検証 の順で進む。
  /// 検証に落ちたときは std::nullopt を返す。
  /// virtual を付けていないので、派生クラスは手順そのものを変えられない。
  std::optional<double> read_once();

  /// 骨格が各段を呼んだ順序の記録。テストが手順の順番を見るために使う。
  const std::vector<std::string> & call_log() const { return call_log_; }

  /// 初期化済みかどうか。
  bool is_initialized() const { return is_initialized_; }

protected:
  SensorReader() = default;

  /// 変換後の値が妥当かを判定する。既定では有限値かどうかだけを見る。
  /// 派生クラスは必要なら差し替えてよい（フックメソッド）。
  ///
  /// これだけ private ではなく protected なのは、
  /// 派生クラスが「基底の判定に足す」形で SensorReader::validate() を呼ぶため。
  /// private virtual はオーバーライドはできても呼び出しはできない。
  virtual bool validate(double converted_value) const;

private:
  /// センサの初期化。read_once() の初回だけ呼ばれる。
  virtual void initialize() = 0;

  /// センサから生の値（AD 値やカウント値）を 1 つ取り出す。
  virtual int fetch_raw() = 0;

  /// 生の値を物理量に変換する。
  virtual double convert(int raw_value) const = 0;

  void record(const char * step_name);

  bool is_initialized_ = false;
  std::vector<std::string> call_log_;
};

/// ロータリエンコーダ。カウント値を角度[deg]に変換する。
class EncoderReader final : public SensorReader
{
public:
  EncoderReader(std::vector<int> samples, double counts_per_revolution);

private:
  void initialize() override;
  int fetch_raw() override;
  double convert(int raw_value) const override;

  std::vector<int> samples_;
  std::size_t next_index_ = 0;
  double counts_per_revolution_;
};

/// サーミスタ。AD 値を温度[degC]に変換し、あり得ない温度を弾く。
class ThermistorReader final : public SensorReader
{
public:
  explicit ThermistorReader(std::vector<int> samples);

  /// 妥当と判定される温度の範囲[degC]。
  static constexpr double kMinValidCelsius = -10.0;
  static constexpr double kMaxValidCelsius = 120.0;

private:
  void initialize() override;
  int fetch_raw() override;
  double convert(int raw_value) const override;
  bool validate(double converted_value) const override;

  std::vector<int> samples_;
  std::size_t next_index_ = 0;
};
