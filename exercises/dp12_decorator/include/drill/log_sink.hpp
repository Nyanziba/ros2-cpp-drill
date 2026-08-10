// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

/// 破棄された順番を記録する場所。
///
/// Decorator は「入れ子になったオブジェクトが、外側を捨てたときに
/// 内側まで正しく解放されるか」が本題です。目で見えないと確認できないので、
/// 各デストラクタがここに自分の名前を書き込みます。
///
/// entries() は関数内 static（第5章 Singleton の Meyers Singleton）です。
class DestructionLog
{
public:
  static std::vector<std::string> & entries()
  {
    static std::vector<std::string> log;
    return log;
  }

  static void record(const std::string & name) { entries().push_back(name); }

  static void clear() { entries().clear(); }
};

/// タグと本文を連結する。
///
/// unique_ptr 版のデコレータとテンプレート版のデコレータで**共有**します。
/// 両方が同じ出力になることを、この 1 関数が保証します。
/// 期待する結果: join_tag("[INFO]", "moving") == "[INFO] moving"
std::string join_tag(const std::string & tag, const std::string & body);

/// Component。結城本の Display に対応します。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須です。無いと、外側のデコレータを delete したときに
///     内側の unique_ptr のデストラクタが走らず、入れ子が丸ごと漏れます。
///   - format() は状態を変えないので const メンバ関数にします。
class LogSink
{
public:
  virtual ~LogSink() = default;

  /// message を整形して返す。
  virtual std::string format(const std::string & message) const = 0;
};

/// ConcreteComponent。何も足さずにそのまま返す、いちばん内側。
class PlainMessage : public LogSink
{
public:
  PlainMessage() = default;
  ~PlainMessage() override;

  std::string format(const std::string & message) const override;
};

/// Decorator（抽象）。結城本の Border に対応します。
///
/// 【所有権】
/// 中身を std::unique_ptr で**所有**します。コンストラクタは値で受け取り、
/// std::move でメンバに入れます。これで「外側を捨てれば内側も消える」が
/// 型に書かれた状態になります。
///
/// Component & で持つ設計（所有しない版）もありますが、その場合は
/// 「中身は外側より長生きさせること」という約束が呼び出し側に残ります。
class SinkDecorator : public LogSink
{
public:
  explicit SinkDecorator(std::unique_ptr<LogSink> inner);
  ~SinkDecorator() override;

protected:
  /// 中身。nullptr を渡していない限り必ず有効です。
  const LogSink & inner() const;

private:
  std::unique_ptr<LogSink> inner_;
};

/// ConcreteDecorator: ログレベルを前に付ける。"[INFO] moving"
class LevelTag : public SinkDecorator
{
public:
  LevelTag(std::unique_ptr<LogSink> inner, std::string level);
  ~LevelTag() override;

  std::string format(const std::string & message) const override;

private:
  std::string level_;
};

/// ConcreteDecorator: 時刻を前に付ける。"12:00:00.000 moving"
///
/// 時刻は引数で受け取ります。テストを決定的にするためで、
/// 実物では std::chrono から作ります（C++編 12章）。
class TimestampTag : public SinkDecorator
{
public:
  TimestampTag(std::unique_ptr<LogSink> inner, std::string stamp);
  ~TimestampTag() override;

  std::string format(const std::string & message) const override;

private:
  std::string stamp_;
};

/// ConcreteDecorator: 発生箇所を前に付ける。"sensor.cpp:42 moving"
class SourceTag : public SinkDecorator
{
public:
  SourceTag(std::unique_ptr<LogSink> inner, std::string file, int line);
  ~SourceTag() override;

  std::string format(const std::string & message) const override;

private:
  std::string file_;
  int line_;
};

/// 組み立てヘルパ。
///
/// make_unique を三重に書くと読みにくいので、包む操作を関数にします。
///   auto sink = with_level(with_timestamp(plain(), "12:00:00.000"), "INFO");
std::unique_ptr<LogSink> plain();
std::unique_ptr<LogSink> with_level(std::unique_ptr<LogSink> inner, std::string level);
std::unique_ptr<LogSink> with_timestamp(std::unique_ptr<LogSink> inner, std::string stamp);
std::unique_ptr<LogSink> with_source(std::unique_ptr<LogSink> inner, std::string file, int line);

// ---------------------------------------------------------------------------
// テンプレート版（マイコン向け）
//
// 入れ子を「型」で作ります。ヒープ確保ゼロ、vtable ゼロ、仮想関数ゼロです。
// 実装はここに全部書いてあります（テンプレートなのでヘッダに置くしかない）。
// 出力が unique_ptr 版と一致するのは、両方が join_tag() を呼んでいるからです。
// ---------------------------------------------------------------------------

/// テンプレート版の ConcreteComponent。
class StaticPlainMessage
{
public:
  std::string format(const std::string & message) const { return message; }
};

/// テンプレート版の LevelTag。Inner を**値で**持ちます。
template <typename Inner>
class StaticLevelTag
{
public:
  StaticLevelTag(Inner inner, std::string level)
  : inner_(std::move(inner)), level_(std::move(level))
  {
  }

  std::string format(const std::string & message) const
  {
    return join_tag("[" + level_ + "]", inner_.format(message));
  }

private:
  Inner inner_;
  std::string level_;
};

/// テンプレート版の TimestampTag。
template <typename Inner>
class StaticTimestampTag
{
public:
  StaticTimestampTag(Inner inner, std::string stamp)
  : inner_(std::move(inner)), stamp_(std::move(stamp))
  {
  }

  std::string format(const std::string & message) const
  {
    return join_tag(stamp_, inner_.format(message));
  }

private:
  Inner inner_;
  std::string stamp_;
};
