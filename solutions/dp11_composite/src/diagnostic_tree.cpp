// 解答例。
//
// 結城本 第11章 Composite。葉（DiagnosticCheck）と節（DiagnosticGroup）を
// DiagnosticEntry として同一視します。
//
// 子の持ち方は std::vector<std::unique_ptr<DiagnosticEntry>> です。
//   - 値で持つ  → スライシングして派生部分が消える（そもそも抽象クラスは持てない）
//   - 生ポインタ → 誰が delete するか型に書かれない
//   - shared_ptr → 共有したい理由が無いので過剰。親子で相互参照すると循環して解放されない

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
  return 1;
}

DiagnosticResult DiagnosticCheck::run() const
{
  DiagnosticResult result;
  if (passes_) {
    result.passed = 1;
  } else {
    result.failed = 1;
  }
  return result;
}

void DiagnosticCheck::collect_names(
  const std::string & prefix, std::vector<std::string> & out) const
{
  out.push_back(prefix + "/" + name());
}

void DiagnosticGroup::add(std::unique_ptr<DiagnosticEntry> child)
{
  if (child == nullptr) {
    return;
  }
  // 値で受けた unique_ptr を、さらにムーブして vector に入れる。
  // ここを std::move し忘れると「コピーできない」とコンパイルエラーになります。
  children_.push_back(std::move(child));
}

std::size_t DiagnosticGroup::check_count() const
{
  std::size_t total = 0;
  for (const std::unique_ptr<DiagnosticEntry> & child : children_) {
    // child が葉かグループかは見ない。見なくて済むのが Composite。
    total += child->check_count();
  }
  return total;
}

DiagnosticResult DiagnosticGroup::run() const
{
  DiagnosticResult total;
  for (const std::unique_ptr<DiagnosticEntry> & child : children_) {
    const DiagnosticResult result = child->run();
    total.passed += result.passed;
    total.failed += result.failed;
  }
  return total;
}

void DiagnosticGroup::collect_names(
  const std::string & prefix, std::vector<std::string> & out) const
{
  const std::string self = prefix + "/" + name();
  out.push_back(self);
  for (const std::unique_ptr<DiagnosticEntry> & child : children_) {
    child->collect_names(self, out);
  }
}

}  // namespace drill
