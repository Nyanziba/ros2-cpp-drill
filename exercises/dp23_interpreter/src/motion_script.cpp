// I AM NOT DONE
//
// 結城本 第23章 Interpreter を C++ で書きます。
// 動作記述ミニ言語 MotionScript を、構文木に落として動作列に展開してください。
//
// 実装するのは 5 つです。
//   1. CommandNode::evaluate()
//   2. SequenceNode::evaluate()
//   3. RepeatNode::evaluate()
//   4. parse()          … 再帰下降パーサ。例外を投げず ParseResult で返す
//   5. run_variant()    … std::variant + std::visit 版の評価
//
// 字句解析（tokenize）は主題ではないので実装済みです。そのまま使ってください。

#include "drill/motion_script.hpp"

#include <cctype>
#include <cstdlib>
#include <utility>

namespace
{

/// 字句。ここは実装済みです。
struct Token
{
  enum class Kind
  {
    Word,        ///< forward / turn / repeat / 未知の語
    Number,      ///< 整数（負も可）
    Semicolon,   ///< ;
    LeftBrace,   ///< {
    RightBrace,  ///< }
    End          ///< 入力の終わり。必ず末尾に 1 つ入ります
  };

  Kind kind = Kind::End;
  std::string text;
  long long number = 0;
  std::size_t position = 0;
};

bool is_word_char(char c)
{
  const unsigned char uc = static_cast<unsigned char>(c);
  return std::isalpha(uc) != 0 || c == '_';
}

bool is_digit_char(char c)
{
  return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

/// ソースを字句に切る。エラーは返しません（知らない文字は Word にして
/// パーサに「未知のコマンド」として弾かせます）。
std::vector<Token> tokenize(const std::string & source)
{
  std::vector<Token> tokens;
  std::size_t index = 0;

  while (index < source.size()) {
    const char c = source[index];
    if (std::isspace(static_cast<unsigned char>(c)) != 0) {
      ++index;
      continue;
    }

    Token token;
    token.position = index;

    if (c == ';') {
      token.kind = Token::Kind::Semicolon;
      token.text = ";";
      ++index;
    } else if (c == '{') {
      token.kind = Token::Kind::LeftBrace;
      token.text = "{";
      ++index;
    } else if (c == '}') {
      token.kind = Token::Kind::RightBrace;
      token.text = "}";
      ++index;
    } else if (is_digit_char(c) ||
               (c == '-' && index + 1 < source.size() && is_digit_char(source[index + 1])))
    {
      const std::size_t begin = index;
      if (c == '-') {
        ++index;
      }
      while (index < source.size() && is_digit_char(source[index])) {
        ++index;
      }
      token.kind = Token::Kind::Number;
      token.text = source.substr(begin, index - begin);
      token.number = std::strtoll(token.text.c_str(), nullptr, 10);
    } else if (is_word_char(c)) {
      const std::size_t begin = index;
      while (index < source.size() && (is_word_char(source[index]) || is_digit_char(source[index]))) {
        ++index;
      }
      token.kind = Token::Kind::Word;
      token.text = source.substr(begin, index - begin);
    } else {
      token.kind = Token::Kind::Word;
      token.text = std::string(1, c);
      ++index;
    }

    tokens.push_back(std::move(token));
  }

  Token end_token;
  end_token.kind = Token::Kind::End;
  end_token.position = source.size();
  tokens.push_back(std::move(end_token));

  return tokens;
}

}  // namespace

void CommandNode::evaluate(std::vector<Motion> & out) const
{
  // TODO: 自分が持っている Motion を 1 つ out の末尾に積んでください。
  (void)out;  // 実装したらこの行は消してください
}

void SequenceNode::evaluate(std::vector<Motion> & out) const
{
  // TODO: 子を先頭から順に evaluate() してください。
  //
  // 子は std::vector<std::unique_ptr<Node>> です。
  // ここで vector を作って結合しないこと。out にそのまま積みます。
  (void)out;  // 実装したらこの行は消してください
}

void RepeatNode::evaluate(std::vector<Motion> & out) const
{
  // TODO: 本体を count_ 回 evaluate() してください。
  //
  // 注意: body_ が nullptr のことは無い前提で書いて構いませんが、
  //       count_ が 0 のときは 1 回も評価しません。
  (void)out;     // 実装したらこの 2 行は消してください
  (void)count_;
}

std::vector<Motion> run(const Node & root)
{
  std::vector<Motion> motions;
  root.evaluate(motions);
  return motions;
}

ParseResult parse(const std::string & source)
{
  const std::vector<Token> tokens = tokenize(source);

  // TODO: 再帰下降パーサを書いてください。
  //
  // 進めかた（この形にすると素直です）:
  //   - 現在位置 index を持つ小さなクラス（または関数群）を無名名前空間に足す
  //   - parse_sequence(depth) : End か '}' に当たるまで parse_statement を繰り返し、
  //                             SequenceNode に append していく
  //   - parse_statement(depth):
  //       "forward" / "turn" → Number を読み、';' を読み、CommandNode を作る
  //       "repeat"           → Number を読み、'{' を読み、
  //                            parse_sequence(depth + 1) を読み、'}' を読む
  //       それ以外           → 「未知のコマンド」エラー
  //
  // エラーの返しかた（**throw しないこと**）:
  //   ParseResult::failure(ParseError{"メッセージ", 位置}) を返します。
  //   途中で失敗したことを上まで伝える必要があるので、
  //   「エラーを保持するメンバ」を 1 つ持って nullptr を返すのが簡単です。
  //
  // 深さの上限:
  //   parse_sequence の入口で depth > kMaxNestingDepth なら
  //   エラーにしてください。ここを見ないと深い入れ子でスタックが尽きます。
  //
  // 数値の範囲:
  //   Token::number は long long です。int に入らない値、
  //   repeat の回数が負、kMaxRepeatCount 超過はエラーにしてください。

  return ParseResult::failure(ParseError{"parse() が未実装です", tokens.back().position});
}

std::vector<Motion> run_variant(const std::vector<variant_ast::VNode> & program)
{
  // TODO: variant 版の評価を書いてください。
  //
  // 各要素は std::variant<Command, std::unique_ptr<Repeat>> です。
  // std::visit にオーバーロードされた呼び出し可能物を渡すか、
  // std::holds_alternative / std::get で分岐します。
  // Repeat のときは body を再帰的に評価して count 回積みます。
  //
  // クラス版の run() と **同じ結果**になることがテストされます。
  std::vector<Motion> motions;
  motions.reserve(program.size());
  return motions;
}
