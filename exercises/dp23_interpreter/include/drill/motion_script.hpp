// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

/// 動作記述ミニ言語 "MotionScript" の構文木と評価。
///
/// 文法（これで全部です）:
///   program   := statement*
///   statement := "forward" NUMBER ";"
///              | "turn"    NUMBER ";"
///              | "repeat"  NUMBER "{" program "}"
///
/// 例:
///   forward 100; turn 90; repeat 3 { forward 50; }
///
/// 【この課題での設計判断】
///   - 結城本は parse() をノード自身に持たせていますが、ここでは
///     **パーサと構文木（AST）を分けます**。AST は「評価する」だけを知り、
///     文字列の読み方を知りません。
///   - 構文エラーは **例外を投げず**、ParseResult という戻り値で返します。
///     マイコンでは -fno-exceptions が普通なので、throw に頼れません。
///   - 再帰下降パーサはネストの深さだけスタックを食います。
///     kMaxNestingDepth で上限を切ります。

/// 展開後の動作 1 つ。
enum class MotionKind
{
  Forward,
  Turn
};

struct Motion
{
  MotionKind kind;
  int value;
};

inline bool operator==(const Motion & lhs, const Motion & rhs)
{
  return lhs.kind == rhs.kind && lhs.value == rhs.value;
}

inline bool operator!=(const Motion & lhs, const Motion & rhs)
{
  return !(lhs == rhs);
}

/// gtest が落ちたときに中身を表示するためのもの。
inline std::ostream & operator<<(std::ostream & os, const Motion & motion)
{
  os << (motion.kind == MotionKind::Forward ? "forward " : "turn ") << motion.value;
  return os;
}

/// 構文木のノード。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。子を unique_ptr で持つので、
///     基底ポインタ経由で必ず破棄されます。
///   - evaluate() は結果を返すのではなく **出力先を受け取ります**。
///     ノードごとに vector を作って結合すると、木の深さぶん確保が走ります。
class Node
{
public:
  virtual ~Node() = default;

  /// この部分木を展開して out の末尾に積む。状態は変えないので const。
  virtual void evaluate(std::vector<Motion> & out) const = 0;
};

/// 終端ノード。forward / turn 1 つぶん。
class CommandNode final : public Node
{
public:
  explicit CommandNode(Motion motion)
  : motion_(motion)
  {
  }

  void evaluate(std::vector<Motion> & out) const override;

  const Motion & motion() const { return motion_; }

private:
  Motion motion_;
};

/// 文の並び。子を所有します（第11章 Composite とまったく同じ構造）。
class SequenceNode final : public Node
{
public:
  void append(std::unique_ptr<Node> child) { children_.push_back(std::move(child)); }

  std::size_t size() const { return children_.size(); }

  void evaluate(std::vector<Motion> & out) const override;

private:
  std::vector<std::unique_ptr<Node>> children_;
};

/// repeat N { ... }。本体を所有します。
class RepeatNode final : public Node
{
public:
  RepeatNode(int count, std::unique_ptr<Node> body)
  : count_(count), body_(std::move(body))
  {
  }

  void evaluate(std::vector<Motion> & out) const override;

private:
  int count_;
  std::unique_ptr<Node> body_;
};

/// 構文エラー。例外の代わりに値で返します。
struct ParseError
{
  std::string message;
  std::size_t position = 0;  ///< source の何バイト目で気づいたか
};

/// parse() の戻り値。成功なら AST を、失敗なら ParseError を持ちます。
/// AST を持つのでコピー不可・ムーブのみです。
class ParseResult
{
public:
  static ParseResult success(std::unique_ptr<Node> ast)
  {
    ParseResult result;
    result.ast_ = std::move(ast);
    return result;
  }

  static ParseResult failure(ParseError error)
  {
    ParseResult result;
    result.error_ = std::move(error);
    return result;
  }

  bool ok() const { return ast_ != nullptr; }

  /// 成功したときだけ有効。失敗なら nullptr。
  const Node * ast() const { return ast_.get(); }

  /// 失敗したときだけ意味があります。
  const ParseError & error() const { return error_; }

private:
  ParseResult() = default;

  std::unique_ptr<Node> ast_;
  ParseError error_;
};

/// 再帰下降パーサが許す入れ子の深さの上限。
/// 深い入れ子をそのまま受けるとスタックを使い切ります。
inline constexpr std::size_t kMaxNestingDepth = 16;

/// repeat に書ける回数の上限。展開結果が爆発しないための歯止めです。
inline constexpr int kMaxRepeatCount = 1000;

/// ソースを構文木にする。例外は投げません。
ParseResult parse(const std::string & source);

/// 構文木を動作列に展開する。
std::vector<Motion> run(const Node & root);

/// ------------------------------------------------------------------
/// std::variant 版（第13章 Visitor と同じ話）
///
/// ノードの種類が forward / turn / repeat の 3 つで固定なら、
/// 継承と vtable は要りません。variant + visit で書けます。
/// ただし **再帰的な型なので間接参照は消えません**。
/// variant の要素は完全型でないといけないので、Repeat は unique_ptr で挟みます。
/// ------------------------------------------------------------------
namespace variant_ast
{

struct Repeat;

struct Command
{
  Motion motion;
};

using VNode = std::variant<Command, std::unique_ptr<Repeat>>;

struct Repeat
{
  int count = 0;
  std::vector<VNode> body;
};

}  // namespace variant_ast

/// variant 版の評価。クラス版の run() と同じ結果になること。
std::vector<Motion> run_variant(const std::vector<variant_ast::VNode> & program);
