// I AM NOT DONE
//
// 結城本 第9章 Bridge を C++ で書きます。
// 「機能のクラス階層」（TelemetryView / RepeatView）と
// 「実装のクラス階層」（TelemetrySink / RecordingSink / NumberedSink）を
// 継承ではなく **1 本のメンバ（unique_ptr<TelemetrySink>）** でつなぎます。

#include "drill/telemetry_view.hpp"

#include <string>

// ── 実装のクラス階層 ────────────────────────────────────────────

void RecordingSink::open()
{
  // TODO: log_ に "<open>" を push_back してください。
  static_cast<void>(log_);
}

void RecordingSink::put_line(const std::string & line)
{
  // TODO: log_ に line をそのまま push_back してください。
  static_cast<void>(line);
}

void RecordingSink::close()
{
  // TODO: log_ に "<close>" を push_back してください。
}

void NumberedSink::open()
{
  // TODO: log_ に "<open>" を push_back し、next_number_ を 0 に戻してください。
  static_cast<void>(log_);
}

void NumberedSink::put_line(const std::string & line)
{
  // TODO: log_ に std::to_string(next_number_) + ": " + line を push_back し、
  //       next_number_ を 1 進めてください。
  //
  // 番号を付けるという「実装側の都合」が、機能側に一切漏れないことを確認してください。
  static_cast<void>(line);
  static_cast<void>(next_number_);
}

void NumberedSink::close()
{
  // TODO: log_ に "<close>" を push_back してください。
}

// ── 機能のクラス階層 ────────────────────────────────────────────

void TelemetryView::show(const std::string & text)
{
  // TODO: sink() に対して open() → put_line(text) → close() の順で呼んでください。
  //
  // ここには「どこへ出るか」が 1 文字も出てきません。それが Bridge です。
  static_cast<void>(text);
}

void RepeatView::show_repeat(const std::string & text)
{
  // TODO: open() は 1 回、put_line(text) を times_ 回、close() は 1 回。
  //
  // 注意: times_ が 0 以下のときも open() と close() は呼びます
  //       （put_line は 0 回）。
  // 注意: 機能を 1 つ増やしましたが、TelemetrySink 側には何も足していません。
  static_cast<void>(text);
  static_cast<void>(times_);
}
