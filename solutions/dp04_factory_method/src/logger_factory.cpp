#include "drill/logger_factory.hpp"

#include <utility>

Logger::Logger(std::string tag, int * alive_counter)
: tag_(std::move(tag)), alive_counter_(alive_counter)
{
  if (alive_counter_ != nullptr) {
    ++(*alive_counter_);
  }
}

Logger::~Logger()
{
  if (alive_counter_ != nullptr) {
    --(*alive_counter_);
  }
}

MemoryLogger::MemoryLogger(std::string tag, int * alive_counter)
: Logger(std::move(tag), alive_counter)
{
}

void MemoryLogger::write(const std::string & message)
{
  lines_.push_back("[" + tag() + "] " + message);
}

std::unique_ptr<Logger> LoggerFactory::create(const std::string & tag)
{
  std::unique_ptr<Logger> logger = create_logger(tag);
  if (logger == nullptr) {
    return nullptr;
  }
  register_logger(*logger);
  return logger;   // ローカル変数なので自動的にムーブされる。std::move は書かない
}

MemoryLoggerFactory::MemoryLoggerFactory(int * alive_counter)
: alive_counter_(alive_counter)
{
}

std::unique_ptr<Logger> MemoryLoggerFactory::create_logger(const std::string & tag)
{
  if (tag.empty()) {
    return nullptr;
  }
  // unique_ptr<MemoryLogger> から unique_ptr<Logger> へは暗黙に変換できる
  return std::make_unique<MemoryLogger>(tag, alive_counter_);
}

void MemoryLoggerFactory::register_logger(const Logger & logger)
{
  registered_tags_.push_back(logger.tag());
}
