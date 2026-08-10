// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace drill
{

/// 診断の集計結果。葉 1 個でも木全体でも同じ型で表します。
struct DiagnosticResult
{
  std::size_t passed = 0;
  std::size_t failed = 0;

  bool all_passed() const { return failed == 0; }
};

/// 破棄されたエントリの名前が push_back される共有ログ。
///
/// 「親を破棄すると子も破棄される」ことをテストから観測するためだけに存在します。
/// 実務のライブラリにこういうグローバルは置かないでください。
std::vector<std::string> & destruction_log();

/// 診断エントリの基底（結城本の Entry に対応）。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。unique_ptr<DiagnosticEntry> で子を持つので、
///     無いと派生のデストラクタが呼ばれません（未定義動作）。
///   - add() を**ここには置きません**。結城本の Entry は add() を持っていて
///     File で例外を投げますが、C++ では「葉に add できない」ことを
///     型で表明できます。add() は DiagnosticGroup にだけあります。
class DiagnosticEntry
{
public:
  virtual ~DiagnosticEntry();

  /// このエントリの名前。木の中で一意である必要はありません。
  const std::string & name() const { return name_; }

  /// このエントリ以下に含まれる「実際に走る診断」の数。
  /// 葉なら 1、グループなら子の合計（グループ自身は数えません）。
  virtual std::size_t check_count() const = 0;

  /// このエントリ以下をすべて実行して合否を集計する。
  virtual DiagnosticResult run() const = 0;

  /// このエントリ以下の名前を prefix 付きで out に積む。
  /// 積む順序は「自分 → 子（登録順）」です。
  virtual void collect_names(const std::string & prefix, std::vector<std::string> & out) const = 0;

  /// collect_names を根から呼ぶだけの補助。仮想ではありません。
  std::vector<std::string> full_names() const
  {
    std::vector<std::string> out;
    collect_names("", out);
    return out;
  }

protected:
  explicit DiagnosticEntry(std::string name)
  : name_(std::move(name))
  {
  }

  DiagnosticEntry(const DiagnosticEntry &) = default;
  DiagnosticEntry(DiagnosticEntry &&) = default;
  DiagnosticEntry & operator=(const DiagnosticEntry &) = default;
  DiagnosticEntry & operator=(DiagnosticEntry &&) = default;

private:
  std::string name_;
};

/// 葉。1 個の診断項目（結城本の File に対応）。
///
/// 実際のハードウェアを触る代わりに、合否を固定値で持ちます。
class DiagnosticCheck : public DiagnosticEntry
{
public:
  DiagnosticCheck(std::string name, bool passes)
  : DiagnosticEntry(std::move(name)),
    passes_(passes)
  {
  }

  ~DiagnosticCheck() override;

  DiagnosticCheck(const DiagnosticCheck &) = delete;
  DiagnosticCheck & operator=(const DiagnosticCheck &) = delete;
  DiagnosticCheck(DiagnosticCheck &&) noexcept = default;
  DiagnosticCheck & operator=(DiagnosticCheck &&) noexcept = default;

  std::size_t check_count() const override;
  DiagnosticResult run() const override;
  void collect_names(const std::string & prefix, std::vector<std::string> & out) const override;

private:
  bool passes_;
};

/// 節。診断項目をまとめたグループ（結城本の Directory に対応）。
///
/// 【所有権】子は unique_ptr で**このグループが所有**します。
/// グループが死ねば、その下の木は丸ごと死にます。親へのポインタは持ちません。
/// 持つなら生ポインタか weak_ptr です（shared_ptr で相互参照すると解放されません）。
class DiagnosticGroup : public DiagnosticEntry
{
public:
  explicit DiagnosticGroup(std::string name)
  : DiagnosticEntry(std::move(name))
  {
  }

  ~DiagnosticGroup() override;

  /// unique_ptr のメンバを持つので**コピーできません**。ムーブだけできます。
  DiagnosticGroup(const DiagnosticGroup &) = delete;
  DiagnosticGroup & operator=(const DiagnosticGroup &) = delete;
  DiagnosticGroup(DiagnosticGroup &&) noexcept = default;
  DiagnosticGroup & operator=(DiagnosticGroup &&) noexcept = default;

  /// 子を追加して所有権を受け取る。
  /// 葉でもグループでも同じ引数で渡せるのが Composite の狙いです。
  /// child が nullptr のときは何もしません。
  void add(std::unique_ptr<DiagnosticEntry> child);

  /// 直下の子の数（再帰しません）。
  std::size_t child_count() const { return children_.size(); }

  std::size_t check_count() const override;
  DiagnosticResult run() const override;
  void collect_names(const std::string & prefix, std::vector<std::string> & out) const override;

private:
  std::vector<std::unique_ptr<DiagnosticEntry>> children_;
};

}  // namespace drill
