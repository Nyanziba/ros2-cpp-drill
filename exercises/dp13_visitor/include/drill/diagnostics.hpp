// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <variant>
#include <vector>

// ============================================================================
// その1: GoF 版の Visitor（二重ディスパッチ）
// ============================================================================
//
// 題材は「起動時セルフチェックの結果ツリー」です。
// グループの下にセンサ検査とモータ検査がぶら下がり、グループは入れ子にできます。
//
// 結城本 第13章の Element / Visitor に対応します。
//   File      → SensorCheck / MotorCheck
//   Directory → CheckGroup
//   Visitor   → DiagVisitor

class SensorCheck;
class MotorCheck;
class CheckGroup;

/// 出力の形。GoF 版と variant 版で同じ文字列を作るために、ここに固定してあります。
namespace diag_format
{

/// 深さに応じたインデント（深さ 1 につき空白 2 個）。
inline std::string indent_of(int depth)
{
  return std::string(static_cast<std::size_t>(depth) * 2U, ' ');
}

/// センサ検査 1 行。例: "bat 11800mV OK\n"
inline std::string sensor_line(const std::string & name, int value_mv, bool failed)
{
  return name + " " + std::to_string(value_mv) + "mV " + (failed ? "NG" : "OK") + "\n";
}

/// モータ検査 1 行。例: "motor_r fault=3 NG\n"
inline std::string motor_line(const std::string & name, unsigned int fault_bits, bool failed)
{
  return name + " fault=" + std::to_string(fault_bits) + " " + (failed ? "NG" : "OK") + "\n";
}

/// グループ 1 行。例: "[drive]\n"
inline std::string group_line(const std::string & name)
{
  return "[" + name + "]\n";
}

}  // namespace diag_format

/// 訪問者。要素の「種類ごと」にオーバーロードされた visit を持ちます。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。
///   - 引数は const 参照。値で受けるとスライシングとコピーが起きます。
class DiagVisitor
{
public:
  virtual ~DiagVisitor() = default;

  virtual void visit(const SensorCheck & node) = 0;
  virtual void visit(const MotorCheck & node) = 0;
  virtual void visit(const CheckGroup & node) = 0;
};

/// 検査結果ツリーの節。
///
/// accept() が「1 回目の仮想呼び出し」です。
/// この 1 回で実行時の型が確定し、その中で呼ぶ visitor.visit(*this) の
/// オーバーロード解決が静的に決まる。これが二重ディスパッチです。
class DiagNode
{
public:
  virtual ~DiagNode() = default;

  DiagNode(const DiagNode &) = delete;
  DiagNode & operator=(const DiagNode &) = delete;

  /// 訪問者を受け入れる。
  virtual void accept(DiagVisitor & visitor) const = 0;

  const std::string & name() const { return name_; }

protected:
  explicit DiagNode(std::string name)
  : name_(std::move(name))
  {
  }

private:
  std::string name_;
};

/// 電圧などのアナログ検査。value_mv が limit_mv を超えたら NG。
class SensorCheck : public DiagNode
{
public:
  SensorCheck(std::string name, int value_mv, int limit_mv)
  : DiagNode(std::move(name)), value_mv_(value_mv), limit_mv_(limit_mv)
  {
  }

  void accept(DiagVisitor & visitor) const override;

  int value_mv() const { return value_mv_; }
  int limit_mv() const { return limit_mv_; }
  bool failed() const { return value_mv_ > limit_mv_; }

private:
  int value_mv_;
  int limit_mv_;
};

/// モータドライバの検査。フォールトビットが 1 つでも立っていたら NG。
class MotorCheck : public DiagNode
{
public:
  MotorCheck(std::string name, unsigned int fault_bits)
  : DiagNode(std::move(name)), fault_bits_(fault_bits)
  {
  }

  void accept(DiagVisitor & visitor) const override;

  unsigned int fault_bits() const { return fault_bits_; }
  bool failed() const { return fault_bits_ != 0U; }

private:
  unsigned int fault_bits_;
};

/// 子を所有するグループ。子の所有権は unique_ptr で持ちます（第11章 Composite と同じ）。
class CheckGroup : public DiagNode
{
public:
  explicit CheckGroup(std::string name)
  : DiagNode(std::move(name))
  {
  }

  void accept(DiagVisitor & visitor) const override;

  /// 子を追加する。所有権を受け取ります。
  void add(std::unique_ptr<DiagNode> child);

  const std::vector<std::unique_ptr<DiagNode>> & children() const { return children_; }

private:
  std::vector<std::unique_ptr<DiagNode>> children_;
};

/// 訪問者その1: NG の数を数える（集計）。
///
/// グループ自身は「訪問した検査」に数えません。子をたどるだけです。
class FailureCountVisitor : public DiagVisitor
{
public:
  void visit(const SensorCheck & node) override;
  void visit(const MotorCheck & node) override;
  void visit(const CheckGroup & node) override;

  /// NG だった検査の数。
  int failure_count() const { return failure_count_; }

  /// 訪問した検査（葉）の数。
  int checked_count() const { return checked_count_; }

private:
  int failure_count_ = 0;
  int checked_count_ = 0;
};

/// 訪問者その2: インデント付きのレポートを作る（整形）。
///
/// 出力の形（行末は必ず "\n"）:
///   グループ  : <インデント>[名前]
///   センサ    : <インデント>名前 12345mV OK
///   モータ    : <インデント>名前 fault=3 NG
/// インデントは深さ 1 につき空白 2 個。ルートは深さ 0。
class TextReportVisitor : public DiagVisitor
{
public:
  void visit(const SensorCheck & node) override;
  void visit(const MotorCheck & node) override;
  void visit(const CheckGroup & node) override;

  const std::string & text() const { return text_; }

  /// 現在の深さ。訪問が終わっていれば 0 に戻っているはずです。
  int depth() const { return depth_; }

private:
  std::string text_;
  int depth_ = 0;
};

// ============================================================================
// その2: std::variant + std::visit 版（継承なし・仮想関数なし・accept なし）
// ============================================================================
//
// 同じ木を、継承を一切使わずに表現します。
// 子は「所有」ではなく DiagArena の添字で指します。こうすると
//   - variant の中に自分自身が入る（再帰型）問題を避けられる
//   - ノード 1 個ごとのヒープ確保が要らない
// ので、マイコン向けにはこの形が扱いやすくなります。

struct SensorSample
{
  std::string name;
  int value_mv = 0;
  int limit_mv = 0;
};

struct MotorSample
{
  std::string name;
  unsigned int fault_bits = 0U;
};

struct GroupSample
{
  std::string name;
  std::vector<std::size_t> children;  ///< DiagArena 内の添字
};

/// 3 種類のうち「ちょうど 1 つ」が入る型。基底クラスも vtable もありません。
using DiagValue = std::variant<SensorSample, MotorSample, GroupSample>;

/// ノードを平らに並べて持つ置き場。木の形は GroupSample::children の添字で表します。
class DiagArena
{
public:
  /// ノードを追加して、その添字を返す。
  std::size_t add(DiagValue value);

  const DiagValue & at(std::size_t index) const { return nodes_.at(index); }

  std::size_t size() const { return nodes_.size(); }

private:
  std::vector<DiagValue> nodes_;
};

/// variant 版の集計。GoF 版の FailureCountVisitor::failure_count() と同じ値を返すこと。
int count_failures(const DiagArena & arena, std::size_t root);

/// variant 版の整形。GoF 版の TextReportVisitor::text() と 1 文字も違わないこと。
std::string make_report(const DiagArena & arena, std::size_t root);
