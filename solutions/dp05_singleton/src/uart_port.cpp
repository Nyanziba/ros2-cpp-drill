// 解答例
//
// 結城本 第5章 Singleton を C++ で書いたもの。
// 要点は 1 つだけです。**Meyers Singleton（関数ローカル static）を使う。**

#include "drill/uart_port.hpp"

namespace
{

/// コンストラクタが走った回数。
///
/// これは定数初期化されるので（0 を書くだけ）、翻訳ユニットをまたいだ
/// 初期化順序の問題は起きません。コンストラクタを持つ型のグローバルだと話が変わります。
int g_uart_construction_count = 0;

/// LazyProbe が一度でも構築されたか。こちらも定数初期化。
bool g_lazy_probe_constructed = false;

}  // namespace

UartPort::UartPort()
{
  // 【マイコンでの注意】
  // 実機ならここでレジスタを叩きたくなりますが、**割り込みの有効化はしません**。
  // このコンストラクタがいつ走るかは「最初に instance() が呼ばれたとき」であって、
  // クロックや NVIC の設定が済んでいる保証がありません。
  // ハードウェアを触る初期化は open() のような明示的な関数に分けます。
  ++g_uart_construction_count;
}

UartPort & UartPort::instance()
{
  // Meyers Singleton。
  //   - 最初にこの行を通ったときにだけ構築される（遅延初期化）
  //   - C++11 以降、この初期化はスレッドセーフだと規格が保証している
  //   - 翻訳ユニットをまたぐ初期化順序の問題が起きない。
  //     「使う側が呼んだ時点」が初期化の時点だから
  static UartPort the_port;
  return the_port;
}

int UartPort::construction_count()
{
  return g_uart_construction_count;
}

void UartPort::set_baud_rate(std::uint32_t baud_rate)
{
  baud_rate_ = baud_rate;
}

std::uint32_t UartPort::baud_rate() const
{
  return baud_rate_;
}

void UartPort::write_line(const std::string & line)
{
  sent_lines_.push_back(line);
}

const std::vector<std::string> & UartPort::sent_lines() const
{
  return sent_lines_;
}

void UartPort::reset()
{
  // オブジェクトは作り直しません。状態だけ構築直後に戻します。
  // 作り直すと instance() が返すアドレスが変わってしまい、
  // 「唯一のインスタンス」という約束が崩れます。
  baud_rate_ = kDefaultBaudRate;
  sent_lines_.clear();
}

LazyProbe::LazyProbe()
{
  g_lazy_probe_constructed = true;
}

LazyProbe & LazyProbe::instance()
{
  static LazyProbe the_probe;
  return the_probe;
}

bool LazyProbe::was_constructed()
{
  return g_lazy_probe_constructed;
}
