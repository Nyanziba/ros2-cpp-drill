// 解答例
//
// 結城本 第9章 Bridge。
// 「機能のクラス階層」と「実装のクラス階層」を、継承ではなく
// 1 本のメンバ（unique_ptr<TelemetrySink>）でつないでいます。

#include "drill/telemetry_view.hpp"

#include <string>

// ── 実装のクラス階層 ────────────────────────────────────────────

void RecordingSink::open()
{
  log_.push_back("<open>");
}

void RecordingSink::put_line(const std::string & line)
{
  log_.push_back(line);
}

void RecordingSink::close()
{
  log_.push_back("<close>");
}

void NumberedSink::open()
{
  log_.push_back("<open>");
  next_number_ = 0;
}

void NumberedSink::put_line(const std::string & line)
{
  log_.push_back(std::to_string(next_number_) + ": " + line);
  ++next_number_;
}

void NumberedSink::close()
{
  log_.push_back("<close>");
}

// ── 機能のクラス階層 ────────────────────────────────────────────

void TelemetryView::show(const std::string & text)
{
  // ここに「どこへ出るか」は 1 文字も出てきません。
  sink().open();
  sink().put_line(text);
  sink().close();
}

void RepeatView::show_repeat(const std::string & text)
{
  // 機能を 1 つ増やしましたが、TelemetrySink 側には何も足していません。
  sink().open();
  for (int i = 0; i < times_; ++i) {
    sink().put_line(text);
  }
  sink().close();
}
