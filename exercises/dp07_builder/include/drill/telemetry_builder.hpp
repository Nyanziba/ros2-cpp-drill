// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <sstream>
#include <string>

/// 数値を文字列にする小道具。
/// テストの期待値を安定させるために、書式をここで固定しています。
///   12.5 -> "12.5" / 3.25 -> "3.25" / 41.0 -> "41"
inline std::string format_number(double value)
{
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

/// 結城本 第7章の Builder（部品を作る側）。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。Director が参照でしか触らなくても、
///     将来 unique_ptr で持った瞬間に事故るので最初から付けます。
///   - Java の `abstract class Builder` はメソッドを空実装できますが、
///     ここでは「必ず実装させる」ために純粋仮想にしています。
class TelemetryBuilder
{
public:
  virtual ~TelemetryBuilder() = default;

  /// 表題を作る。
  virtual void make_header(const std::string & title) = 0;

  /// 項目を 1 つ足す。
  virtual void make_field(const std::string & key, double value) = 0;

  /// 締める。
  virtual void make_footer() = 0;
};

/// 結城本 第7章の Director（手順を持つ側）。
///
/// 「何を作るか」は Builder が、「どういう順で作るか」は Director が持ちます。
/// Director は具体的な出力形式を一切知りません。
///
/// 【寿命の約束】
/// builder_ は参照です。Director は Builder より長生きさせられません。
/// Java なら GC が Builder を生かしますが、C++ では宙に浮きます。
class TelemetryDirector
{
public:
  explicit TelemetryDirector(TelemetryBuilder & builder)
  : builder_(builder)
  {
  }

  /// 決められた手順でテレメトリを 1 件組み立てる。
  ///
  /// 手順は次で固定です（テストもこの順序を見ます）。
  ///   1. make_header("robot telemetry")
  ///   2. make_field("battery_voltage", 12.5)
  ///   3. make_field("motor_current", 3.25)
  ///   4. make_field("cpu_temperature", 41.0)
  ///   5. make_footer()
  void construct();

private:
  TelemetryBuilder & builder_;
};

/// CSV 形式で組み立てる ConcreteBuilder。
///
/// 期待する出力:
///   # robot telemetry
///   battery_voltage,12.5
///   motor_current,3.25
///   cpu_temperature,41
///   # end
class CsvTelemetryBuilder : public TelemetryBuilder
{
public:
  void make_header(const std::string & title) override;
  void make_field(const std::string & key, double value) override;
  void make_footer() override;

  /// 出来上がりを取り出す。
  ///
  /// この関数は Builder の共通インタフェース（TelemetryBuilder）には
  /// **置けません**。戻り型が形式ごとに違うからです。
  /// 結城本でも getResult() は ConcreteBuilder 側にあります。
  const std::string & result() const { return text_; }

private:
  std::string text_;
};

/// JSON 形式で組み立てる ConcreteBuilder。
///
/// 期待する出力:
///   {
///     "title": "robot telemetry",
///     "battery_voltage": 12.5,
///     "motor_current": 3.25,
///     "cpu_temperature": 41
///   }
class JsonTelemetryBuilder : public TelemetryBuilder
{
public:
  void make_header(const std::string & title) override;
  void make_field(const std::string & key, double value) override;
  void make_footer() override;

  const std::string & result() const { return text_; }

private:
  std::string text_;
};
