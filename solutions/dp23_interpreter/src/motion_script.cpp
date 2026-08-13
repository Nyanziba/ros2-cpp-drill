// 解答例。
//
// 結城本 第23章 Interpreter。
// ポイントは 3 つです。
//   - AST（Node の木）とパーサを分けている。AST は文字列を知らない
//   - 構文エラーは throw せず ParseResult で返す
//   - 再帰下降の入口で深さを見る（見ないと深い入れ子でスタックが尽きる）

#include "drill/motion_script.hpp"

#include <cctype>
#include <cstdlib>
#include <limits>
#include <utility>

namespace
{

/// 字句。
struct Token
{
  enum class Kind
  {
    Word,
    Number,
    Semicolon,
    LeftBrace,
    RightBrace,
    End
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

/// 再帰下降パーサ。
///
/// エラーは例外ではなくメンバ error_ に置き、関数は nullptr を返します。
/// 「失敗したら nullptr、詳細はメンバ」は C++ で throw を使わないときの定番です。
class Parser
{
public:
  explicit Parser(std::vector<Token> tokens)
  : tokens_(std::move(tokens))
  {
  }

  ParseResult parse_program()
  {
    std::unique_ptr<Node> program = parse_sequence(0);
    if (program == nullptr) {
      return ParseResult::failure(std::move(error_));
    }
    if (peek().kind != Token::Kind::End) {
      // '}' が余っている場合はここに来ます。
      return ParseResult::failure(
        ParseError{"対応する repeat が無い '" + peek().text + "' があります", peek().position});
    }
    return ParseResult::success(std::move(program));
  }

private:
  const Token & peek() const { return tokens_[index_]; }

  const Token & consume() { return tokens_[index_++]; }

  std::nullptr_t fail(std::string message, std::size_t position)
  {
    error_ = ParseError{std::move(message), position};
    return nullptr;
  }

  std::unique_ptr<Node> parse_sequence(std::size_t depth)
  {
    // ここを最初に見るのが肝心です。
    // 深さを見ずに再帰すると、深い入れ子でスタックを使い切って落ちます。
    if (depth > kMaxNestingDepth) {
      return fail("repeat の入れ子が深すぎます", peek().position);
    }

    auto sequence = std::make_unique<SequenceNode>();
    while (peek().kind != Token::Kind::End && peek().kind != Token::Kind::RightBrace) {
      std::unique_ptr<Node> statement = parse_statement(depth);
      if (statement == nullptr) {
        return nullptr;
      }
      sequence->append(std::move(statement));
    }
    return sequence;
  }

  std::unique_ptr<Node> parse_statement(std::size_t depth)
  {
    const Token & head = peek();
    if (head.kind != Token::Kind::Word) {
      return fail("コマンド名が要るところに '" + head.text + "' があります", head.position);
    }

    if (head.text == "forward" || head.text == "turn") {
      return parse_command();
    }
    if (head.text == "repeat") {
      return parse_repeat(depth);
    }
    return fail("未知のコマンド '" + head.text + "'", head.position);
  }

  std::unique_ptr<Node> parse_command()
  {
    const Token & name = consume();
    const MotionKind kind = (name.text == "forward") ? MotionKind::Forward : MotionKind::Turn;

    int value = 0;
    if (!read_int(&value)) {
      return nullptr;
    }
    if (!expect(Token::Kind::Semicolon, ";")) {
      return nullptr;
    }
    return std::make_unique<CommandNode>(Motion{kind, value});
  }

  std::unique_ptr<Node> parse_repeat(std::size_t depth)
  {
    const Token & keyword = consume();

    int count = 0;
    if (!read_int(&count)) {
      return nullptr;
    }
    if (count < 0) {
      return fail("repeat の回数が負です", keyword.position);
    }
    if (count > kMaxRepeatCount) {
      return fail("repeat の回数が大きすぎます", keyword.position);
    }
    if (!expect(Token::Kind::LeftBrace, "{")) {
      return nullptr;
    }

    std::unique_ptr<Node> body = parse_sequence(depth + 1);
    if (body == nullptr) {
      return nullptr;
    }
    if (!expect(Token::Kind::RightBrace, "}")) {
      return nullptr;
    }
    return std::make_unique<RepeatNode>(count, std::move(body));
  }

  bool expect(Token::Kind kind, const char * what)
  {
    if (peek().kind != kind) {
      fail(std::string("'") + what + "' が要るところに '" + peek().text + "' があります",
           peek().position);
      return false;
    }
    consume();
    return true;
  }

  bool read_int(int * out)
  {
    if (peek().kind != Token::Kind::Number) {
      fail("数値が要るところに '" + peek().text + "' があります", peek().position);
      return false;
    }
    const Token & token = consume();
    if (token.number < std::numeric_limits<int>::min() ||
        token.number > std::numeric_limits<int>::max())
    {
      fail("数値が大きすぎます", token.position);
      return false;
    }
    *out = static_cast<int>(token.number);
    return true;
  }

  std::vector<Token> tokens_;
  std::size_t index_ = 0;
  ParseError error_;
};

/// variant 版の評価。std::visit に渡すためのオーバーロード束ね。
struct VariantEvaluator
{
  std::vector<Motion> * out;

  void operator()(const variant_ast::Command & command) const
  {
    out->push_back(command.motion);
  }

  void operator()(const std::unique_ptr<variant_ast::Repeat> & repeat) const
  {
    if (repeat == nullptr) {
      return;
    }
    for (int i = 0; i < repeat->count; ++i) {
      for (const variant_ast::VNode & child : repeat->body) {
        std::visit(*this, child);
      }
    }
  }
};

}  // namespace

void CommandNode::evaluate(std::vector<Motion> & out) const
{
  out.push_back(motion_);
}

void SequenceNode::evaluate(std::vector<Motion> & out) const
{
  for (const std::unique_ptr<Node> & child : children_) {
    child->evaluate(out);
  }
}

void RepeatNode::evaluate(std::vector<Motion> & out) const
{
  for (int i = 0; i < count_; ++i) {
    body_->evaluate(out);
  }
}

std::vector<Motion> run(const Node & root)
{
  std::vector<Motion> motions;
  root.evaluate(motions);
  return motions;
}

ParseResult parse(const std::string & source)
{
  Parser parser{tokenize(source)};
  return parser.parse_program();
}

std::vector<Motion> run_variant(const std::vector<variant_ast::VNode> & program)
{
  std::vector<Motion> motions;
  const VariantEvaluator evaluator{&motions};
  for (const variant_ast::VNode & node : program) {
    std::visit(evaluator, node);
  }
  return motions;
}
