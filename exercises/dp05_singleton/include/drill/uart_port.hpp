// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// 既定のボーレート。マジックナンバーを直接書かないための定数。
constexpr std::uint32_t kDefaultBaudRate = 115200;

/// ロボットに 1 本しかない UART をモデル化したもの。
///
/// 【なぜこれはシングルトンでよいのか】
/// 「設定を持ち回るのが面倒だから」ではありません。
/// **UART1 という物理的なペリフェラルが世界に 1 個しか無い**からです。
/// 2 個目を構築できてしまうと、同じレジスタを 2 つのオブジェクトが
/// 別々の状態だと思い込んで叩くことになります。
/// この理由が言えないものはシングルトンにしないでください
/// （docs/patterns/00_使う前に.md の 0.2）。
///
/// 【Java 版との差が出るところ】
///   - コピー・ムーブを明示的に禁止しないと `UartPort p = UartPort::instance();`
///     が通ってしまい、シングルトンでなくなります。Java では起きない事故です。
///   - コンストラクタは private。外から作らせません。
///   - instance() は **参照** を返します。ポインタだと呼ぶ側が
///     delete してよいのか分からなくなります。
class UartPort
{
public:
  /// 唯一のインスタンスを返す。
  /// 何度呼んでも同じオブジェクトで、初期化は最初の呼び出しのときだけ走ります。
  static UartPort & instance();

  // シングルトンで最重要の 4 行。消すとコピーが作れてしまいます。
  UartPort(const UartPort &) = delete;
  UartPort & operator=(const UartPort &) = delete;
  UartPort(UartPort &&) = delete;
  UartPort & operator=(UartPort &&) = delete;

  /// ボーレートを設定する。
  void set_baud_rate(std::uint32_t baud_rate);

  /// 現在のボーレート。
  std::uint32_t baud_rate() const;

  /// 1 行送信する（実機の代わりに送信履歴に積むだけ）。
  void write_line(const std::string & line);

  /// これまでに送信した行。
  const std::vector<std::string> & sent_lines() const;

  /// 状態を構築直後まで戻す。
  ///
  /// 【これは何のためにあるのか】
  /// シングルトンは状態がテストからテストへ漏れます。
  /// 前のテストが書いたボーレートが次のテストに残るのは、テストではなく
  /// 「前のテストの続き」です。そこで各テストの先頭で reset() を呼びます。
  /// **オブジェクトは作り直しません。状態だけ戻します。**
  /// 実機コードからは呼ばないでください。
  void reset();

  /// コンストラクタが走った回数。
  /// 「初期化が一度だけ」であることをテストから確かめるために公開しています。
  static int construction_count();

private:
  UartPort();

  std::uint32_t baud_rate_ = kDefaultBaudRate;
  std::vector<std::string> sent_lines_;
};

/// 「初期化が最初の instance() まで走らない（遅延初期化）」ことだけを見るための型。
///
/// UartPort と分けてあるのは、UartPort は他のテストが先に触ってしまうため、
/// 「まだ構築されていない」状態を観測できないからです。
class LazyProbe
{
public:
  static LazyProbe & instance();

  LazyProbe(const LazyProbe &) = delete;
  LazyProbe & operator=(const LazyProbe &) = delete;
  LazyProbe(LazyProbe &&) = delete;
  LazyProbe & operator=(LazyProbe &&) = delete;

  /// 一度でも構築されたか。
  static bool was_constructed();

private:
  LazyProbe();
};
