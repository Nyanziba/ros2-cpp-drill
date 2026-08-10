// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

/// ── 実装のクラス階層 ──────────────────────────────────────────
///
/// 「テレメトリを 1 行どこへ出すか」だけを担当します。
/// 何を出すか（機能）は一切知りません。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。TelemetryView が unique_ptr<TelemetrySink> で
///     持つので、無いと派生のデストラクタが呼ばれません（未定義動作）。
class TelemetrySink
{
public:
  virtual ~TelemetrySink() = default;

  /// 出力を開始する。
  virtual void open() = 0;

  /// 1 行出す。
  virtual void put_line(const std::string & line) = 0;

  /// 出力を終える。
  virtual void close() = 0;
};

/// 実装その1: 受け取った行をそのまま記録する。
///
/// 記録先は外から渡された vector です。参照で持つので、
/// **log は RecordingSink より長生きさせてください。**
class RecordingSink : public TelemetrySink
{
public:
  explicit RecordingSink(std::vector<std::string> & log)
  : log_(log)
  {
  }

  void open() override;
  void put_line(const std::string & line) override;
  void close() override;

private:
  std::vector<std::string> & log_;
};

/// 実装その2: 行頭に 0 始まりの通し番号を付けて記録する。
///
/// RecordingSink とインタフェースが同じなので、機能側から見ると区別が付きません。
/// **この 2 つを入れ替えても、機能側のコードは 1 行も変わりません。**
class NumberedSink : public TelemetrySink
{
public:
  explicit NumberedSink(std::vector<std::string> & log)
  : log_(log)
  {
  }

  void open() override;
  void put_line(const std::string & line) override;
  void close() override;

private:
  std::vector<std::string> & log_;
  std::size_t next_number_ = 0;
};

/// ── 機能のクラス階層 ──────────────────────────────────────────
///
/// 「何を出すか」を担当します。どこへ出るかは知りません。
/// 実装への橋（Bridge）は sink_ という **1 本のメンバ**です。継承ではありません。
///
/// 誰が sink を所有するか: TelemetryView です。だから unique_ptr で受け取ります。
class TelemetryView
{
public:
  explicit TelemetryView(std::unique_ptr<TelemetrySink> sink)
  : sink_(std::move(sink))
  {
  }

  /// 機能側も派生されるので仮想デストラクタが要ります。
  virtual ~TelemetryView() = default;

  TelemetryView(const TelemetryView &) = delete;
  TelemetryView & operator=(const TelemetryView &) = delete;

  /// text を 1 回だけ出す。open() → put_line() → close() の順で実装を呼びます。
  void show(const std::string & text);

protected:
  /// 派生（機能側）が実装を触るための入口。
  TelemetrySink & sink() { return *sink_; }

private:
  std::unique_ptr<TelemetrySink> sink_;
};

/// 機能を 1 つ増やしたもの（結城本の CountDisplay に対応）。
///
/// **TelemetrySink 側には何も足していません。** これが「2 軸で独立に増やせる」の意味です。
class RepeatView : public TelemetryView
{
public:
  RepeatView(std::unique_ptr<TelemetrySink> sink, int times)
  : TelemetryView(std::move(sink)),
    times_(times)
  {
  }

  /// text を times 回出す。open() は 1 回、close() も 1 回です。
  void show_repeat(const std::string & text);

private:
  int times_;
};
