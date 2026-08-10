// I AM NOT DONE
//
// 結城本 第13章 Visitor を C++ で書きます。
//   その1: GoF 版の二重ディスパッチ（accept → visit）
//   その2: std::variant + std::visit + overloaded イディオム
// 同じ木に対して、2 つの版が 1 文字も違わない結果を返すところまで作ります。

#include "drill/diagnostics.hpp"

#include <string>
#include <utility>

// 出力の形は drill/diagnostics.hpp の diag_format にまとめてあります。
// GoF 版と variant 版でまったく同じ文字列を作るために、必ずこれを使ってください。
using diag_format::group_line;
using diag_format::indent_of;
using diag_format::motor_line;
using diag_format::sensor_line;

// ---------------------------------------------------------------------------
// GoF 版: accept が 1 回目の仮想呼び出し
// ---------------------------------------------------------------------------

void SensorCheck::accept(DiagVisitor & visitor) const
{
  // TODO: visitor.visit(*this); と書いてください。
  //
  // 3 つの accept はどれも字面がまったく同じになります。
  // それでも基底クラスに 1 個書いてまとめることはできません。
  // 基底の中では *this の静的型が DiagNode になり、
  // DiagNode を取る visit が存在しないのでコンパイルが通りません。
  // 「実行時の型を静的型に戻す」ための 1 回目の仮想呼び出しが、この accept です。
  (void)visitor;
}

void MotorCheck::accept(DiagVisitor & visitor) const
{
  // TODO: SensorCheck::accept と同じです。
  (void)visitor;
}

void CheckGroup::accept(DiagVisitor & visitor) const
{
  // TODO: ここも同じです。子をたどるのは訪問者側の仕事なので、ここでは何もしません。
  (void)visitor;
}

void CheckGroup::add(std::unique_ptr<DiagNode> child)
{
  // TODO: children_ に move で入れてください。所有権はグループが持ちます。
  (void)child;
}

// ---------------------------------------------------------------------------
// 訪問者その1: 集計
// ---------------------------------------------------------------------------

void FailureCountVisitor::visit(const SensorCheck & node)
{
  // TODO: checked_count_ を 1 増やし、node.failed() なら failure_count_ も増やす。
  (void)node;
}

void FailureCountVisitor::visit(const MotorCheck & node)
{
  // TODO: SensorCheck 版と同じです。
  (void)node;
}

void FailureCountVisitor::visit(const CheckGroup & node)
{
  // TODO: node.children() を回して child->accept(*this) を呼んでください。
  //
  // 木をたどるのは要素側ではなく訪問者側の仕事です（結城本の ListVisitor と同じ）。
  // 要素側に走査を書くと、訪問順を変えたい訪問者が書けなくなります。
  (void)node;
}

// ---------------------------------------------------------------------------
// 訪問者その2: 整形
// ---------------------------------------------------------------------------

void TextReportVisitor::visit(const SensorCheck & node)
{
  // TODO: text_ に indent_of(depth_) + sensor_line(...) を足してください。
  (void)node;
}

void TextReportVisitor::visit(const MotorCheck & node)
{
  // TODO: motor_line を使って同様に。
  (void)node;
}

void TextReportVisitor::visit(const CheckGroup & node)
{
  // TODO:
  //   1. text_ に indent_of(depth_) + group_line(node.name()) を足す
  //   2. depth_ を 1 増やす
  //   3. 子を accept する
  //   4. depth_ を 1 戻す
  (void)node;
}

// ---------------------------------------------------------------------------
// variant 版（継承なし・仮想関数なし・accept なし）
// ---------------------------------------------------------------------------

std::size_t DiagArena::add(DiagValue value)
{
  // TODO: nodes_ の末尾に move で入れて、その添字（= 追加後の size() - 1）を返す。
  (void)value;
  return 0U;
}

namespace
{

// TODO: overloaded イディオムをここに書いてください。
//
//   template <class ... Ts>
//   struct overloaded : Ts ...
//   {
//     using Ts::operator() ...;
//   };
//
//   template <class ... Ts>
//   overloaded(Ts ...) -> overloaded<Ts ...>;
//
// 複数のラムダを継承でまとめ、using で全部の operator() を可視にする道具です。
// C++17 では推論ガイド（下の 2 行）を自分で書く必要があります。C++20 では不要です。

}  // namespace

int count_failures(const DiagArena & arena, std::size_t root)
{
  // TODO: std::visit と overloaded を使って、NG の数を数えてください。
  //   SensorSample → value_mv > limit_mv なら 1
  //   MotorSample  → fault_bits != 0 なら 1
  //   GroupSample  → children の添字それぞれについて再帰して合計
  //
  // 再帰が要るので、ラムダの中から呼べる名前付きの関数を無名名前空間に用意すると楽です。
  (void)arena;
  (void)root;
  return 0;
}

std::string make_report(const DiagArena & arena, std::size_t root)
{
  // TODO: count_failures と同じ要領で、GoF 版とまったく同じ文字列を組み立ててください。
  //   SensorSample → indent_of(depth) + sensor_line(...)
  //   MotorSample  → indent_of(depth) + motor_line(...)
  //   GroupSample  → indent_of(depth) + "[" + name + "]\n" のあと、子を depth + 1 で
  (void)arena;
  (void)root;
  return std::string{};
}
