// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>
#include <string>
#include <vector>

/// Product: ログの出力先。
///
/// alive_counter は「今いくつ生きているか」を外から観測するための非所有ポインタ。
/// 所有権の移動をテストで確認するためだけに置いてある。nullptr を渡してよい。
class Logger
{
public:
  Logger(std::string tag, int * alive_counter);
  virtual ~Logger();

  Logger(const Logger &) = delete;
  Logger & operator=(const Logger &) = delete;

  const std::string & tag() const { return tag_; }

  /// 1 行書く。
  virtual void write(const std::string & message) = 0;

private:
  std::string tag_;
  int * alive_counter_;
};

/// ConcreteProduct: メモリ上に貯めるロガー。
class MemoryLogger : public Logger
{
public:
  MemoryLogger(std::string tag, int * alive_counter);

  void write(const std::string & message) override;

  /// 書かれた行。書式は "[タグ] メッセージ"。
  const std::vector<std::string> & lines() const { return lines_; }

private:
  std::vector<std::string> lines_;
};

/// Creator: 生成の「手順」を持つ。手順は固定で、作るものだけを派生に任せる。
/// Factory Method は Template Method の一種であることが、この形に出ている。
class LoggerFactory
{
public:
  LoggerFactory() = default;
  virtual ~LoggerFactory() = default;

  LoggerFactory(const LoggerFactory &) = delete;
  LoggerFactory & operator=(const LoggerFactory &) = delete;

  /// テンプレートメソッド。非仮想。
  ///
  /// 手順:
  ///   1. create_logger() で作る
  ///   2. nullptr（生成失敗）ならそのまま nullptr を返す。登録はしない
  ///   3. register_logger() で登録する
  ///   4. 所有権ごと呼び出し側へ返す
  std::unique_ptr<Logger> create(const std::string & tag);

private:
  /// FactoryMethod: 何を作るかだけを決める。
  /// 作れないときは nullptr を返す（例外は投げない）。
  virtual std::unique_ptr<Logger> create_logger(const std::string & tag) = 0;

  /// 作ったものを記録する。所有権は受け取らないので const 参照で渡す。
  virtual void register_logger(const Logger & logger) = 0;
};

/// ConcreteCreator: MemoryLogger を作る。
class MemoryLoggerFactory : public LoggerFactory
{
public:
  explicit MemoryLoggerFactory(int * alive_counter = nullptr);

  /// create() が成功したタグが、成功した順に並ぶ。
  const std::vector<std::string> & registered_tags() const { return registered_tags_; }

private:
  std::unique_ptr<Logger> create_logger(const std::string & tag) override;
  void register_logger(const Logger & logger) override;

  int * alive_counter_;
  std::vector<std::string> registered_tags_;
};
