// 解答例。
//
// 結城本 第12章 Decorator。Border/Display を LogSink/SinkDecorator に置き換えています。
// C++ での要点は 3 つです。
//   1. 中身は std::unique_ptr で所有する（コンストラクタは値で受けて std::move）
//   2. 基底に仮想デストラクタ。無いと入れ子の内側が解放されない
//   3. 組み立ては全部ムーブ。コピーしようとした時点でコンパイルエラーになる

#include "drill/log_sink.hpp"

std::string join_tag(const std::string & tag, const std::string & body)
{
  return tag + " " + body;
}

PlainMessage::~PlainMessage()
{
  DestructionLog::record("PlainMessage");
}

std::string PlainMessage::format(const std::string & message) const
{
  return message;
}

SinkDecorator::SinkDecorator(std::unique_ptr<LogSink> inner)
: inner_(std::move(inner))
{
}

SinkDecorator::~SinkDecorator() = default;

const LogSink & SinkDecorator::inner() const
{
  return *inner_;
}

LevelTag::LevelTag(std::unique_ptr<LogSink> inner, std::string level)
: SinkDecorator(std::move(inner)), level_(std::move(level))
{
}

LevelTag::~LevelTag()
{
  DestructionLog::record("LevelTag");
}

std::string LevelTag::format(const std::string & message) const
{
  // 先に中身へ仕事をさせ、その結果を包む。ここが Decorator の中心。
  return join_tag("[" + level_ + "]", inner().format(message));
}

TimestampTag::TimestampTag(std::unique_ptr<LogSink> inner, std::string stamp)
: SinkDecorator(std::move(inner)), stamp_(std::move(stamp))
{
}

TimestampTag::~TimestampTag()
{
  DestructionLog::record("TimestampTag");
}

std::string TimestampTag::format(const std::string & message) const
{
  return join_tag(stamp_, inner().format(message));
}

SourceTag::SourceTag(std::unique_ptr<LogSink> inner, std::string file, int line)
: SinkDecorator(std::move(inner)), file_(std::move(file)), line_(line)
{
}

SourceTag::~SourceTag()
{
  DestructionLog::record("SourceTag");
}

std::string SourceTag::format(const std::string & message) const
{
  return join_tag(file_ + ":" + std::to_string(line_), inner().format(message));
}

std::unique_ptr<LogSink> plain()
{
  return std::make_unique<PlainMessage>();
}

std::unique_ptr<LogSink> with_level(std::unique_ptr<LogSink> inner, std::string level)
{
  return std::make_unique<LevelTag>(std::move(inner), std::move(level));
}

std::unique_ptr<LogSink> with_timestamp(std::unique_ptr<LogSink> inner, std::string stamp)
{
  return std::make_unique<TimestampTag>(std::move(inner), std::move(stamp));
}

std::unique_ptr<LogSink> with_source(std::unique_ptr<LogSink> inner, std::string file, int line)
{
  return std::make_unique<SourceTag>(std::move(inner), std::move(file), line);
}
