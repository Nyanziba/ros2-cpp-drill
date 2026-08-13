// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "drill/motion_script.hpp"

namespace
{

Motion forward(int value)
{
  return Motion{MotionKind::Forward, value};
}

Motion turn(int value)
{
  return Motion{MotionKind::Turn, value};
}

/// パースして展開するところまで。失敗したらテストを落とす。
std::vector<Motion> expand(const std::string & source)
{
  const ParseResult result = parse(source);
  EXPECT_TRUE(result.ok()) << "parse に失敗: " << result.error().message;
  if (!result.ok()) {
    return {};
  }
  return run(*result.ast());
}

/// repeat を n 重に入れ子にしたソースを作る。
std::string nested_source(std::size_t depth)
{
  std::string source;
  for (std::size_t i = 0; i < depth; ++i) {
    source += "repeat 1 { ";
  }
  source += "forward 1; ";
  for (std::size_t i = 0; i < depth; ++i) {
    source += "} ";
  }
  return source;
}

}  // namespace

TEST(InterpreterTest, 単純な文の並びを解釈する)
{
  const std::vector<Motion> motions = expand("forward 100; turn 90; forward 50;");

  const std::vector<Motion> expected = {forward(100), turn(90), forward(50)};
  EXPECT_EQ(motions, expected);
}

TEST(InterpreterTest, 負の数と改行を含む入力を解釈する)
{
  const std::vector<Motion> motions = expand("turn -90;\n  forward 0;\n");

  const std::vector<Motion> expected = {turn(-90), forward(0)};
  EXPECT_EQ(motions, expected);
}

TEST(InterpreterTest, 空のプログラムは空の動作列になる)
{
  const ParseResult result = parse("   \n  ");
  ASSERT_TRUE(result.ok()) << result.error().message;
  ASSERT_NE(result.ast(), nullptr);
  EXPECT_TRUE(run(*result.ast()).empty());
}

TEST(InterpreterTest, 繰り返しが展開される)
{
  const std::vector<Motion> motions = expand("repeat 3 { forward 50; }");

  const std::vector<Motion> expected = {forward(50), forward(50), forward(50)};
  EXPECT_EQ(motions, expected);
}

TEST(InterpreterTest, 繰り返し0回は何も生まない)
{
  EXPECT_TRUE(expand("repeat 0 { forward 50; turn 90; }").empty());
}

TEST(InterpreterTest, 入れ子の繰り返しが正しく展開される)
{
  const std::vector<Motion> motions =
    expand("forward 10; repeat 2 { turn 90; repeat 2 { forward 5; } } turn -90;");

  const std::vector<Motion> expected = {
    forward(10),
    turn(90), forward(5), forward(5),
    turn(90), forward(5), forward(5),
    turn(-90)};
  EXPECT_EQ(motions, expected);
}

TEST(InterpreterTest, 構文エラーは例外ではなくエラー値で返る)
{
  const std::vector<std::string> broken = {
    "forward;",             // 数値が無い
    "forward 100",          // セミコロンが無い
    "dance 3;",             // 未知のコマンド
    "repeat 2 { forward 10;",  // 閉じ括弧が無い
    "repeat 2 forward 10;",    // 開き括弧が無い
    "repeat -1 { forward 1; }",  // 回数が負
    "}",                    // 対応しない閉じ括弧
  };

  for (const std::string & source : broken) {
    ParseResult result = ParseResult::failure(ParseError{"未実行", 0});
    ASSERT_NO_THROW(result = parse(source)) << "throw してはいけません: " << source;
    EXPECT_FALSE(result.ok()) << "エラーになるはず: " << source;
    EXPECT_EQ(result.ast(), nullptr) << source;
    EXPECT_FALSE(result.error().message.empty())
      << "エラーメッセージが空です: " << source;
  }

  // 「常に失敗する」パーサでは意味が無いので、正しい入力も見ます。
  EXPECT_TRUE(parse("forward 1; repeat 1 { turn 1; }").ok());
}

TEST(InterpreterTest, 正しい入力ではエラーにならない)
{
  const ParseResult result = parse("repeat 2 { forward 1; turn 1; }");
  EXPECT_TRUE(result.ok()) << result.error().message;
  EXPECT_NE(result.ast(), nullptr);
}

TEST(InterpreterTest, variant版がクラス版と同じ結果を返す)
{
  using namespace variant_ast;

  // repeat 2 { turn 90; repeat 2 { forward 5; } }
  auto inner = std::make_unique<Repeat>();
  inner->count = 2;
  inner->body.push_back(Command{forward(5)});

  auto outer = std::make_unique<Repeat>();
  outer->count = 2;
  outer->body.push_back(Command{turn(90)});
  outer->body.push_back(std::move(inner));

  std::vector<VNode> program;
  program.push_back(Command{forward(10)});
  program.push_back(std::move(outer));
  program.push_back(Command{turn(-90)});

  const std::vector<Motion> from_variant = run_variant(program);
  const std::vector<Motion> from_classes =
    expand("forward 10; repeat 2 { turn 90; repeat 2 { forward 5; } } turn -90;");

  EXPECT_EQ(from_variant, from_classes);
  EXPECT_EQ(from_variant.size(), 8u);
}

TEST(InterpreterTest, variant版が空とコマンド1つを正しく扱う)
{
  const std::vector<variant_ast::VNode> empty_program;
  EXPECT_TRUE(run_variant(empty_program).empty());

  std::vector<variant_ast::VNode> one;
  one.push_back(variant_ast::Command{turn(45)});

  const std::vector<Motion> expected = {turn(45)};
  EXPECT_EQ(run_variant(one), expected);
}

TEST(InterpreterTest, 上限までの入れ子は通る)
{
  const std::vector<Motion> motions = expand(nested_source(kMaxNestingDepth));

  const std::vector<Motion> expected = {forward(1)};
  EXPECT_EQ(motions, expected);
}

TEST(InterpreterTest, 上限を超える入れ子はスタックを壊さずエラーになる)
{
  // 上限ちょうどは通ること。ここが落ちるなら制限が厳しすぎます。
  EXPECT_TRUE(parse(nested_source(kMaxNestingDepth)).ok());

  ParseResult result = ParseResult::failure(ParseError{"未実行", 0});
  ASSERT_NO_THROW(result = parse(nested_source(kMaxNestingDepth + 1)));
  EXPECT_FALSE(result.ok()) << "深さ制限が効いていません";
  EXPECT_FALSE(result.error().message.empty());

  // 1000 重でも落ちないこと（再帰の入口で深さを見ていれば落ちません）。
  ASSERT_NO_THROW(result = parse(nested_source(1000)));
  EXPECT_FALSE(result.ok());
}

TEST(InterpreterTest, 繰り返し回数の上限を超えるとエラーになる)
{
  const std::string source =
    "repeat " + std::to_string(static_cast<long long>(kMaxRepeatCount) + 1) + " { forward 1; }";

  const ParseResult result = parse(source);
  EXPECT_FALSE(result.ok()) << "kMaxRepeatCount を超えた回数はエラーにしてください";

  // ちょうど上限は通ること。
  const std::string just_under =
    "repeat " + std::to_string(kMaxRepeatCount) + " { forward 1; }";
  const ParseResult accepted = parse(just_under);
  ASSERT_TRUE(accepted.ok()) << accepted.error().message;
  EXPECT_EQ(run(*accepted.ast()).size(), static_cast<std::size_t>(kMaxRepeatCount));
}
