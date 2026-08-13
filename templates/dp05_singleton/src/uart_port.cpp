// I AM NOT DONE
//
// 結城本 第5章 Singleton を C++ で書きます。
//
// 実装するのは 5 か所です。すべて「関数ローカル static」（Meyers Singleton）が鍵です。
//   1. UartPort::instance()
//   2. UartPort::reset()
//   3. UartPort::construction_count()
//   4. LazyProbe::instance()
//   5. LazyProbe::was_constructed()
//
// ヘッダ include/drill/uart_port.hpp は編集しません。
// なぜコピー・ムーブが = delete されているのかは、ヘッダのコメントを読んでください。

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
  // ここは実装済みです。触らないでください。
  //
  // 【マイコンでの注意】
  // 実機ならここでレジスタを叩きたくなりますが、**割り込みの有効化はしません**。
  // このコンストラクタがいつ走るかは「最初に instance() が呼ばれたとき」であって、
  // クロックや NVIC の設定が済んでいる保証がありません。
  // ハードウェアを触る初期化は open() のような明示的な関数に分けます。
  ++g_uart_construction_count;
}

UartPort & UartPort::instance()
{
  // TODO: 唯一のインスタンスへの参照を返してください。
  //
  // ヒント: 関数ローカル static を 1 つ置くだけです。
  //         C++11 以降、関数ローカル static の初期化は
  //         **スレッドセーフであることが規格で保証されています**（マジックスタティック）。
  //         自分でロックを書く必要はありません。
  //
  // 今の実装は呼ばれるたびに新しいオブジェクトを作っています。
  // 「instance() が常に同じ物を返す」テストと
  // 「初期化が一度だけ走る」テストが落ちます。
  UartPort * port = new UartPort();
  return *port;
}

int UartPort::construction_count()
{
  // TODO: g_uart_construction_count を返してください。
  return -1;
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
  // TODO: 状態を「構築直後」に戻してください。
  //       baud_rate_ を kDefaultBaudRate に、sent_lines_ を空にします。
  //
  // 注意: オブジェクトを作り直すのではありません。**状態だけ**戻します。
  //       作り直せてしまうと、instance() が返すアドレスが変わってしまいます。
}

LazyProbe::LazyProbe()
{
  // ここは実装済みです。
  g_lazy_probe_constructed = true;
}

LazyProbe & LazyProbe::instance()
{
  // TODO: UartPort::instance() と同じく、関数ローカル static にしてください。
  //
  // 今の実装は呼ばれるたびに new しています。
  LazyProbe * probe = new LazyProbe();
  return *probe;
}

bool LazyProbe::was_constructed()
{
  // TODO: g_lazy_probe_constructed を返してください。
  return true;
}
