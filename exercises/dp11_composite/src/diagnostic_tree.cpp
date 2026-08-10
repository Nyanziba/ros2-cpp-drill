// I AM NOT DONE
//
// 結城本 第11章 Composite を C++ で書きます。
// 葉（DiagnosticCheck）と節（DiagnosticGroup）を DiagnosticEntry として
// 同一視し、再帰的に集計できるようにしてください。
//
// この章の難所は所有権です。子は
//   std::vector<std::unique_ptr<DiagnosticEntry>> children_
// で持ちます。値で持つとスライシングし、生ポインタで持つと誰が解放するか
// 型に書かれません。

#include "drill/diagnostic_tree.hpp"

#include <utility>

namespace drill
{

std::vector<std::string> & destruction_log()
{
  static std::vector<std::string> log;
  return log;
}

DiagnosticEntry::~DiagnosticEntry() = default;

// 以下 2 つのデストラクタは実装済みです（テストが破棄の順序を観測するため）。
// 親が死ねば children_ の unique_ptr が死に、その先の子も死にます。
// この連鎖を自分で書く必要はありません。それが unique_ptr を使う理由です。
DiagnosticCheck::~DiagnosticCheck()
{
  destruction_log().push_back(name());
}

DiagnosticGroup::~DiagnosticGroup()
{
  destruction_log().push_back(name());
}

std::size_t DiagnosticCheck::check_count() const
{
  // TODO: 葉は「実際に走る診断」1 個です。いくつ返せばよいでしょうか。
  return 0;
}

DiagnosticResult DiagnosticCheck::run() const
{
  // TODO: passes_ が true なら passed を 1、false なら failed を 1 にして返してください。
  //
  // 葉と節が同じ DiagnosticResult を返すので、呼ぶ側は
  // 「これが 1 個の診断なのか、1000 個をまとめたグループなのか」を知らずに済みます。
  static_cast<void>(passes_);
  return DiagnosticResult{};
}

void DiagnosticCheck::collect_names(
  const std::string & prefix, std::vector<std::string> & out) const
{
  // TODO: prefix + "/" + name() を out に push_back してください。
  static_cast<void>(prefix);
  static_cast<void>(out);
}

void DiagnosticGroup::add(std::unique_ptr<DiagnosticEntry> child)
{
  // TODO: child を children_ に入れてください。
  //
  // 引数は値で受けています。unique_ptr はコピーできないので、
  // 呼び出し側は必ずムーブして渡すことになります。
  // ここでも children_.push_back(std::move(child)) とムーブして入れます。
  // child が nullptr のときは何もしないでください。
  static_cast<void>(child);
}

std::size_t DiagnosticGroup::check_count() const
{
  // TODO: 子の check_count() をすべて足して返してください。
  //
  // 子が葉なのかグループなのかは見ません。見なくて済むのが Composite です。
  // グループ自身は「実際に走る診断」ではないので数えません。
  return 0;
}

DiagnosticResult DiagnosticGroup::run() const
{
  // TODO: 子の run() を登録順に呼び、passed と failed を合計して返してください。
  return DiagnosticResult{};
}

void DiagnosticGroup::collect_names(
  const std::string & prefix, std::vector<std::string> & out) const
{
  // TODO: 自分の名前（prefix + "/" + name()）を先に積み、
  //       そのあと各子の collect_names を「自分のフルパス」を prefix にして呼んでください。
  static_cast<void>(prefix);
  static_cast<void>(out);
}

}  // namespace drill
