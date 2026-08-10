// I AM NOT DONE
//
// Factory Method。生成物の所有権を unique_ptr で呼び出し側に渡します。

#include "drill/logger_factory.hpp"

#include <utility>

Logger::Logger(std::string tag, int * alive_counter)
: tag_(std::move(tag)), alive_counter_(alive_counter)
{
  // TODO: alive_counter_ が nullptr でなければ 1 増やしてください。
  (void)alive_counter_;   // 実装したらこの行は消してください
}

Logger::~Logger()
{
  // TODO: alive_counter_ が nullptr でなければ 1 減らしてください。
  (void)alive_counter_;   // 実装したらこの行は消してください
}

MemoryLogger::MemoryLogger(std::string tag, int * alive_counter)
: Logger(std::move(tag), alive_counter)
{
}

void MemoryLogger::write(const std::string & message)
{
  // TODO: "[タグ] メッセージ" の形で 1 行を lines_ に追加してください。
  //       タグは tag() で取れます。例: tag() が "motor" なら "[motor] duty=0.5"
  (void)message;
}

std::unique_ptr<Logger> LoggerFactory::create(const std::string & tag)
{
  // TODO: テンプレートメソッドを書いてください。
  //   1. create_logger(tag) で作る
  //   2. nullptr なら、登録せずにそのまま nullptr を返す
  //   3. register_logger(*logger) で登録する
  //   4. 所有権ごと返す（unique_ptr はコピーできないので、そのまま return する）
  (void)tag;
  return nullptr;
}

MemoryLoggerFactory::MemoryLoggerFactory(int * alive_counter)
: alive_counter_(alive_counter)
{
}

std::unique_ptr<Logger> MemoryLoggerFactory::create_logger(const std::string & tag)
{
  // TODO: std::make_unique<MemoryLogger>(tag, alive_counter_) で作って返してください。
  //       ただし tag が空文字列のときは生成失敗とみなして nullptr を返します。
  (void)tag;
  (void)alive_counter_;   // 実装したらこの行は消してください
  return nullptr;
}

void MemoryLoggerFactory::register_logger(const Logger & logger)
{
  // TODO: logger.tag() を registered_tags_ に追加してください。
  (void)logger;
}
