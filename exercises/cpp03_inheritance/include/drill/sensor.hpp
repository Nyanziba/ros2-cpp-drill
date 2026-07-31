// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>
#include <string>
#include <vector>

/// 抽象基底クラス: センサの インタフェース。
class Sensor
{
public:
  virtual ~Sensor() = default;

  /// センサの値を読む（純粋仮想関数 - 必ず実装を強制する）。
  virtual double read() const = 0;

  /// センサの名前（仮想関数 - デフォルト実装あり）。
  virtual std::string label() const;
};

/// 温度センサ（Sensor を継承）。
/// TODO: Sensor を継承して read() を override で実装してください。
///       read() は常に 25.0 を返します。label() は override しません（デフォルトを使用）。
class TemperatureSensor : public Sensor
{
public:
  double read() const override;
};

/// 湿度センサ（Sensor を継承）。
/// TODO: Sensor を継承して read() を override で実装してください。
///       read() は常に 60.0 を返します。label() を override して "HumiditySensor" を返します。
class HumiditySensor : public Sensor
{
public:
  double read() const override;
  std::string label() const override;
};
