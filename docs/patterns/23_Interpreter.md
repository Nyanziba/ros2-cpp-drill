# 23. Interpreter

> **結城本 第23章 対応。** `Node` / `ProgramNode` / `CommandListNode` / `RepeatCommandNode` と、
> `Context`（`nextToken()` を持つやつ）を手元に開いてください。
>
> **この章のねらい**: Interpreter は **23 個のうちいちばん出番が少ないパターン**です。
> まず「入れないための判断」から始めます。そのうえで、C++ で書くときに実際に効く 3 点
> ——**構文木の所有権**、**パーサと AST を分ける**、**例外を使わずに構文エラーを返す**——
> をやります。最後に `std::variant` 版と並べて、第13章 Visitor の話に繋げます。

## 23.1 まず、入れない判断

結城本の第23章は面白い章ですが、**部活のライブラリで自作パーサを書く場面はほぼ有りません。**
先にそこを潰します。

Interpreter を入れる前に、この 3 つを順に聞いてください。

| 聞くこと | Yes なら |
| --- | --- |
| その「言語」は設定を書くだけか | **YAML / JSON のパーサを使う。** 自作しない |
| 種類が有限で、コンパイル時に決まっているか | `enum` と配列で足りる。パースは要らない |
| 実行時に**文字列から**振る舞いを組み立てる必要が本当にあるか | No なら、関数を直接呼べばいい |

具体的に言うと、こうです。

- **ロボットのパラメータ**（PID ゲイン、リミット値）→ YAML。ROS 2 なら
  `declare_parameter` があります。ミニ言語を作る理由がありません
- **動作シーケンス**（前進 → 旋回 → 停止）→ まず `std::vector<Motion>` を
  コードで直に書けないか考えます。書けるなら言語は要りません
- **試合中に PC から動作を投げ込みたい** → ここで初めて「テキストを解釈する」話になります。
  それでも、まずバイナリのコマンド列（`uint8_t opcode; int16_t arg;`）で足りないか考えます。
  マイコン側でテキストをパースするより、**PC 側で解析してバイナリで送る**方がほぼ常に正解です

「じゃあ学ぶ意味は」と言われたら、答えは 1 つです。

> **木構造を再帰的に評価する、という考え方**を身につけるため。

構文木の評価は、パーサ以外にも出てきます。ビヘイビアツリー、状態機械の階層、
UI のレイアウト計算、シーングラフの座標合成。どれも
「ノードが自分を評価し、子に同じことを頼む」という同じ形です。
**Interpreter を書くのは、その形を一度手で書くためです。**
文字列を読む部分（パーサ）はおまけです。

[0. 使う前に](00_使う前に.md) の 4 つのチェックのうち、この章に効くのは 4 番
（標準ライブラリに同じものが無いか）です。**設定ファイルなら、既にあります。**

## 23.2 Java 版をそのまま C++ にすると

結城本の `Node` はこうです。

```java
public abstract class Node {
    public abstract void parse(Context context) throws ParseException;
}
```

C++ に素直に移すとこうなります。……が、**3 か所変えないと C++ では成立しません。**

```cpp
class Node
{
public:
  virtual ~Node() = default;                              // 変更点1
  virtual void evaluate(std::vector<Motion> & out) const = 0;  // 変更点2, 3
};
```

### 変更点1: 仮想デストラクタ

この章の木は `std::unique_ptr<Node>` で子を持ちます。**基底ポインタ経由で破棄されます。**
仮想デストラクタが無ければ、`RepeatNode` が持っていた子の木がまるごと漏れます。
第1章から23章まで同じ話です。最後まで同じでした。

### 変更点2: `parse()` をノードから外した

結城本の `Node` は `parse()` を持っています。**C++ ではこれをやりません。**
理由は 23.4 で書きます。ここでは「`Node` に残すのは `evaluate()` だけ」とだけ。

### 変更点3: 戻り値ではなく出力先を受け取る

「評価結果を返す」と書くと、こうなります。

```cpp
virtual std::vector<Motion> evaluate() const = 0;   // 素直だが遅い
```

`RepeatNode` が 3 回まわすと、子の `vector` が 3 個できて、それを連結した `vector` がもう 1 個できます。
**木の深さぶん確保とコピーが積み重なります。** Java でも同じことは起きますが、
Java は `ArrayList` の参照を返すだけなので目立ちません。C++ は値を返すので露骨に出ます。

```cpp
virtual void evaluate(std::vector<Motion> & out) const = 0;   // 確保は 1 回
```

出力先を非 const 参照で渡します。呼ぶ側が 1 個 `vector` を作り、木全体がそこに積みます。
**`const` を付ける場所にも注意**してください。ノード自身は変わらないので `evaluate` は `const`、
`out` は書き込むので `const` を付けません。Java にはこの区別がありません。

## 23.3 誰が所有するのか — 木そのもの

Java 版は `new ProgramNode()` して、参照が張られて、GC が回収します。
C++ では所有を型に書きます。**答えは第11章 Composite とまったく同じ**です。

```cpp
class SequenceNode final : public Node
{
public:
  void append(std::unique_ptr<Node> child) { children_.push_back(std::move(child)); }
  void evaluate(std::vector<Motion> & out) const override;

private:
  std::vector<std::unique_ptr<Node>> children_;   // 親が子を単独所有する
};

class RepeatNode final : public Node
{
public:
  RepeatNode(int count, std::unique_ptr<Node> body)
  : count_(count), body_(std::move(body)) {}

private:
  int count_;
  std::unique_ptr<Node> body_;
};
```

**構文木は所有権がいちばん簡単な木です。** 親が子を 1 人で持ち、共有はありません。
`shared_ptr` は要りません。根を 1 個捨てれば、全部消えます。

```cpp
{
  ParseResult result = parse(source);
  // ...
}   // 木がまるごと解放される
```

ここで **`final`** を付けているのは、これ以上派生させないことを型で言うためです。
付けるとデストラクタの仮想呼び出しをコンパイラが省ける場合があります。無料です。

> **注意**: 深い木を捨てると、**デストラクタも再帰します**。
> `~SequenceNode` → `~unique_ptr` → 子の `~Node` → … と潜ります。
> 23.6 の深さ制限は、実は評価だけでなく破棄も守っています。

## 23.4 パーサと AST を分ける

結城本は `Node::parse(Context)` を持たせています。**これは Java でもやや古い書き方**で、
C++ では分けるほうが素直です。理由は 2 つあります。

**理由1: 責務が 2 つある。** `Node` が「文字列の読み方」と「評価のしかた」を両方知っています。
言語の文法を変えると評価コードのあるファイルが全部変わります。逆に、
評価だけ変えたい（動作列ではなくログ文字列を吐きたい）ときにパーサを読む羽目になります。

**理由2: テストが書けない。** 分けてあれば、パーサを通さずに木を直接組んで評価だけ試せます。

```cpp
auto body = std::make_unique<SequenceNode>();
body->append(std::make_unique<CommandNode>(Motion{MotionKind::Forward, 50}));
RepeatNode repeat{3, std::move(body)};

std::vector<Motion> out;
repeat.evaluate(out);          // パーサを一切通さずに評価だけ検証できる
```

逆に、パーサだけを「木の形が正しいか」で試すこともできます。
**課題のテストが `std::variant` 版と結果を突き合わせられるのも、この分離のおかげ**です。

分けた結果、こういう形になります。

```
文字列 ──tokenize──> トークン列 ──parse──> AST ──evaluate──> 動作列
```

各段が独立していて、それぞれ単体で試せます。
`Context` に相当するもの（現在位置）は、`Node` ではなく**パーサが持ちます**。

## 23.5 構文エラーを例外なしで返す

Java 版は `throws ParseException` です。C++ でそのまま `throw` すると、
**マイコンで動きません**（`-fno-exceptions`）。値で返します。

```cpp
struct ParseError
{
  std::string message;
  std::size_t position = 0;   // 何バイト目で気づいたか。これが無いと使い物にならない
};

class ParseResult
{
public:
  static ParseResult success(std::unique_ptr<Node> ast);
  static ParseResult failure(ParseError error);

  bool ok() const { return ast_ != nullptr; }
  const Node * ast() const { return ast_.get(); }
  const ParseError & error() const { return error_; }

private:
  std::unique_ptr<Node> ast_;
  ParseError error_;
};
```

`std::optional<std::unique_ptr<Node>>` でも書けますが、**失敗の理由が消えます。**
構文エラーは「どこで何が」を伝えないと直せません。`std::optional` は
「値が無い理由が 1 つしかないとき」に使うものです。ここでは自前の型にします。

> C++23 なら `std::expected<std::unique_ptr<Node>, ParseError>` がそのままこれです。
> C++17 の範囲では自分で書きます。**書く形は `expected` に寄せておく**と、
> あとで置き換えられます。

### 失敗をどう上に伝えるか

再帰下降パーサは関数が入れ子に呼び合います。`throw` を使わないなら、伝える手段が要ります。
定番はこれです。

```cpp
class Parser
{
private:
  std::unique_ptr<Node> parse_statement(std::size_t depth);   // 失敗したら nullptr
  std::nullptr_t fail(std::string message, std::size_t position)
  {
    error_ = ParseError{std::move(message), position};
    return nullptr;                                            // そのまま return できる
  }

  ParseError error_;
};
```

**「戻り値は成否だけ、詳細はメンバに置く」**。`errno` と同じ発想ですが、
グローバルではなくパーサのメンバなので、スレッドの問題も起きません。
各呼び出し元は `if (child == nullptr) { return nullptr; }` を書くだけです。

エラーメッセージは**その場で作る**こと。上に戻ってから作ろうとすると、
どのトークンで失敗したのかが分からなくなります。

## 23.6 再帰下降パーサはスタックを食う

C++ 固有というより、**マイコンで致命的になる**話です。

再帰下降パーサは、入れ子 1 段につきスタックフレームを 1 つ積みます。
`repeat 1 { repeat 1 { ... } }` を 20 万段書かれると、**入力が正しくても落ちます。**

23.9 で実際に落とします。ここでは対処だけ。

```cpp
inline constexpr std::size_t kMaxNestingDepth = 16;

std::unique_ptr<Node> Parser::parse_sequence(std::size_t depth)
{
  if (depth > kMaxNestingDepth) {                 // ★ 再帰の「入口」で見る
    return fail("repeat の入れ子が深すぎます", peek().position);
  }
  // ...
  parse_sequence(depth + 1);
}
```

**入口で見ること。** 出口や後始末で見ても、そこに着く前にスタックが尽きます。
そして深さは**引数で渡します**。メンバのカウンタにすると、
早期 return のたびに減算を忘れてズレます。

上限値は「実用上ありえない深さ」で切ります。動作記述に 16 段の入れ子は来ません。
**上限が低すぎて困ることより、上限が無くて落ちることの方がずっと痛い**です。

同じ理由で、`repeat` の回数にも上限を置きます（課題では `kMaxRepeatCount = 1000`）。
`repeat 1000000 { forward 1; }` は構文としては正しく、展開すると 100 万要素です。
**入力の妥当性は、構文だけでなく規模でも見ます。**

## 23.7 `std::variant` で書く — 第13章 Visitor の続き

ノードの種類が**固定**なら（この課題では forward / turn / repeat の 3 つ）、
継承も vtable も要りません。第13章でやった話がそのまま使えます。

```cpp
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
```

**`unique_ptr` が消えていないことに注目してください。**
「variant にすればヒープが消える」と思いがちですが、**再帰的な型では消えません。**
`Repeat` は自分自身を含みうるので、どこかで間接参照が必要です。
さらに `std::variant` の要素は完全型でなければならないので、
`std::variant<Command, Repeat>` とは書けません（`Repeat` の定義中に `Repeat` のサイズが要る）。
`unique_ptr` を挟むと `Repeat` が不完全型のままで通ります。

評価はこう書きます。

```cpp
struct VariantEvaluator
{
  std::vector<Motion> * out;

  void operator()(const Command & command) const { out->push_back(command.motion); }

  void operator()(const std::unique_ptr<Repeat> & repeat) const
  {
    for (int i = 0; i < repeat->count; ++i) {
      for (const VNode & child : repeat->body) {
        std::visit(*this, child);      // 自分を渡して再帰
      }
    }
  }
};
```

継承版との比較です。

| | 継承 + `unique_ptr<Node>` | `std::variant` |
| --- | --- | --- |
| 種類を増やす | ノードを 1 つ足すだけ。既存コードは無傷 | `variant` と**全部の visitor**を直す |
| 操作を増やす（評価以外に整形も） | 全ノードに仮想関数を足す | visitor を 1 個足すだけ |
| 網羅性チェック | 無い（実装忘れは実行時に発覚） | **コンパイルエラーになる** |
| ヒープ | 必ず要る | 再帰する型なら結局要る |
| vtable | ある | 無い（代わりに index による分岐） |

**種類が増えるなら継承、操作が増えるなら variant。** これが第13章の結論でした。
ミニ言語の構文は普通そう頻繁には増えないので、**Interpreter は variant 向き**です。
課題では両方書いて、同じ結果になることを確認します。

## 23.8 標準ライブラリ／言語機能に同じものが無いか

「文字列を解釈する」という意味では、**あります。用途を選べば自作は要りません。**

| やりたいこと | 使うもの |
| --- | --- |
| 数値を読む | `std::stoi` / `std::from_chars`（例外なし・確保なし） |
| 空白区切りの値を読む | `std::istringstream` |
| 正規表現 | `std::regex`（**ただし遅く、確保も例外も走る**。マイコンでは使わない） |
| 設定ファイル | YAML / JSON のライブラリ。自作しない |
| コマンドライン | 各種の引数パーサ |

「構文木を評価する」ほうは、**標準にはありません。**
`std::visit` は分岐の道具であって木ではありません。ここは自分で書く部分です。

なお `std::function` を組み合わせて木の代わりにする手もあります
（第22章 Command と同じ発想で、ノードの代わりにクロージャを積む）。
**単純な言語ならこちらの方が短い**です。ただしヒープを踏みます。

## 23.9 手元で試す

「深さを見ない再帰下降パーサは本当に落ちるのか」を、実際に落として確認します。
**実行する前に、何が起きるか予想してください。**

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Node
{
public:
  virtual ~Node() = default;
  virtual void evaluate(std::vector<int> & out) const = 0;
};

class Leaf final : public Node
{
public:
  explicit Leaf(int value) : value_(value) {}
  void evaluate(std::vector<int> & out) const override { out.push_back(value_); }

private:
  int value_;
};

class Repeat final : public Node
{
public:
  Repeat(int count, std::unique_ptr<Node> body)
  : count_(count), body_(std::move(body)) {}

  void evaluate(std::vector<int> & out) const override
  {
    for (int i = 0; i < count_; ++i) {
      body_->evaluate(out);
    }
  }

private:
  int count_;
  std::unique_ptr<Node> body_;
};

// 深さを一切見ない再帰下降パーサ。"(" が来たら再帰する。
std::unique_ptr<Node> parse(const std::string & source, std::size_t & index)
{
  if (index < source.size() && source[index] == '(') {
    ++index;
    std::unique_ptr<Node> body = parse(source, index);
    if (index < source.size() && source[index] == ')') {
      ++index;
    }
    return std::make_unique<Repeat>(2, std::move(body));
  }
  return std::make_unique<Leaf>(1);
}

std::string nested(std::size_t depth)
{
  return std::string(depth, '(') + std::string(depth, ')');
}

int main()
{
  std::size_t index = 0;
  const std::string small = nested(3);
  const std::unique_ptr<Node> tree = parse(small, index);

  std::vector<int> out;
  tree->evaluate(out);
  std::cout << "depth 3 -> " << out.size() << " leaves\n";

  std::cout << "parsing depth 200000 ..." << std::endl;
  std::size_t deep_index = 0;
  const std::string deep = nested(200000);
  const std::unique_ptr<Node> deep_tree = parse(deep, deep_index);
  std::cout << "parsed. (ここに来たら幸運)" << std::endl;
  return deep_tree == nullptr ? 1 : 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 1 行目は何と出るか。そして 200000 段はどうなるか</summary>

手元（macOS / Apple clang）での実際の出力です。

```
depth 3 -> 8 leaves
parsing depth 200000 ...
```

そして終了コードは **139**（`128 + 11`、つまり SIGSEGV）です。
`parsed.` は表示されません。**構文としては完全に正しい入力で、パーサがスタックを踏み抜きました。**

1 行目が 8 なのは `2^3` です。`Repeat{2}` が 3 段入れ子で、葉が 1 個。
**入力の長さは深さに比例なのに、展開結果は指数で増える**ことも見えます。
`repeat` の回数と深さの両方に上限が要る理由がこれです。

なお、仮に `parse` が生き延びても、次は `deep_tree` の**デストラクタが再帰**して同じ場所で落ちます。
木は作るのも壊すのも再帰です。

</details>

## 23.10 マイコンでの結論

**実行時にテキストをパースしない。** これが結論です。理由は 23.6 と、確保と、例外です。

- 再帰下降 = スタックを食う。**RAM が数十 KB の世界でスタック使用量が入力依存になるのは論外**
- AST = `unique_ptr` の木 = ヒープ。ループ中に確保が走る
- `std::string` のエラーメッセージ = さらに確保
- `throw` できない

設定は**コンパイル時に決める**か、**バイナリのテーブルで持ちます**。

```cpp
// 動作シーケンスは constexpr の配列で持つ。パースはゼロ。
struct Step
{
  MotionKind kind;
  int16_t value;
};

constexpr Step kApproachSequence[] = {
  {MotionKind::Forward, 100},
  {MotionKind::Turn, 90},
  {MotionKind::Forward, 50},
};
// ROM に置かれる。RAM も確保も使わない
```

PC 側から動作を投げ込みたいなら、**テキストではなくバイナリのコマンド列**にします。

```cpp
// 固定長・再帰なし。opcode を見て 1 命令ずつ実行するだけ
struct Instruction
{
  uint8_t opcode;   // 0: forward, 1: turn, 2: repeat_begin, 3: repeat_end
  int16_t arg;
};

// repeat は「木」ではなく「スタック」で扱う。深さの上限が配列サイズで決まる
class Machine
{
public:
  bool run(const Instruction * program, std::size_t count)
  {
    std::size_t loop_depth = 0;
    for (std::size_t pc = 0; pc < count; ++pc) {
      switch (program[pc].opcode) {
        case 2:
          if (loop_depth >= kMaxLoopDepth) {
            return false;                       // 深すぎ。例外は投げない
          }
          loop_stack_[loop_depth].start = pc;
          loop_stack_[loop_depth].remaining = program[pc].arg;
          ++loop_depth;
          break;
        case 3:
          // 残り回数を減らして start に戻る（省略）
          break;
        default:
          execute(program[pc]);
          break;
      }
    }
    return true;
  }

private:
  static constexpr std::size_t kMaxLoopDepth = 4;
  struct Loop { std::size_t start; int remaining; };
  Loop loop_stack_[kMaxLoopDepth] = {};

  void execute(const Instruction & instruction);
};
```

ポイントは 3 つです。

1. **木を作らない。** 命令列を前から舐めるだけ。ヒープゼロ
2. **再帰しない。** 入れ子はループスタック（固定長配列）で表す。**深さの上限が型に書かれる**
3. **エラーは `bool` / エラーコード。** 文字列も例外も使わない

テキストの解析が要るなら、**PC 側でやってこの `Instruction` 配列を送ります。**
マイコンにパーサは載せません。

## 23.11 ROS 2 での結論（補足）

ROS 2 側では実行時パースは自由に書けます。ただし、**その前に既存のものを探してください。**

- パラメータ → `declare_parameter` + YAML。ミニ言語を作る理由になりません
- 動作の記述 → BehaviorTree.CPP（Nav2 が使っています）。XML でツリーを書き、
  ノードを C++ で登録します。**まさに Interpreter ですが、既に有ります**
- メッセージ定義 → `.msg` / `.srv` の IDL とそのコンパイラが既に有ります

自作パーサを書く前に、「これは BehaviorTree.CPP でできないか」を必ず聞いてください。
新しいライブラリを入れる話になるので、そこは人に相談してから決めます。

## 23.12 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| 深い入れ子で SIGSEGV（終了コード 139） | 再帰の入口で深さを見ていない。23.6 |
| 木を捨てるときに落ちる | デストラクタも再帰する。深さ制限は破棄も守っている |
| 展開結果が巨大になってメモリを食う | `repeat` の回数に上限が無い。入力は規模でも検証する |
| エラーの位置が分からず直せない | `ParseError` に `position` が無い。メッセージだけでは足りない |
| `repeat` の本体が外の文まで食う | `parse_sequence` を `}` と入力終端の両方で止めていない |
| ノードを増やしたら評価を書き忘れた | 継承版では検出できない。`variant` + `visit` なら**コンパイルエラー** |
| `std::variant<Command, Repeat>` がコンパイルできない | 再帰的で不完全型。`unique_ptr` を挟む |
| `std::visit` で数十行のテンプレートエラー | visitor に候補型のうち 1 つ分の `operator()` が無い |
| 評価が遅い／確保が多い | `evaluate()` が `std::vector` を返している。出力先を渡す |
| 木が漏れる | `Node` に仮想デストラクタが無い |

## 23.13 対応する課題

```bash
./drill run dp23
```

`exercises/dp23_interpreter/src/motion_script.cpp` に、動作記述ミニ言語
`forward 100; turn 90; repeat 3 { forward 50; }` の解釈を実装します。

1. `CommandNode` / `SequenceNode` / `RepeatNode` の `evaluate()`
2. `parse()` — 再帰下降パーサ。**例外を投げず** `ParseResult` で返す。**深さ制限つき**
3. `run_variant()` — `std::variant` + `std::visit` 版の評価

テストは、入れ子の展開結果、構文エラーがエラー値で返ること、
`variant` 版がクラス版と一致すること、そして
**1000 段の入れ子を投げても落ちずにエラーになること**まで見ます。

## 23.14 この章のまとめ

- **まず入れない判断。** 設定なら YAML/JSON。部活で自作パーサを書く場面はほぼ無い
- 学ぶ価値は「**木を再帰的に評価する**」という形。ビヘイビアツリーもシーングラフも同じ形
- 構文木は `std::unique_ptr` の木。**所有が最も単純な木**で、`shared_ptr` は要らない
- **`parse()` はノードに持たせない。** パーサと AST を分けると、責務が割れてテストできる
- 構文エラーは**例外ではなく値**で返す。`position` を必ず入れる。C++23 なら `std::expected`
- 再帰下降は**入口で深さを見る**。見ないと正しい入力でスタックを踏み抜く（実測: 終了コード 139）
- 種類が固定なら `std::variant`。ただし**再帰的な型では間接参照は消えない**
- マイコンでは**実行時パースをしない**。固定長の命令列 + ループスタックで、再帰も確保もゼロ

---

## この講習を終えて

23 章、おつかれさまでした。最後に、全体を通して見えたことをまとめます。

### 繰り返し出てきた論点は 4 つだった

パターンは 23 個ありましたが、**C++ で毎回問われたことは 4 つ**でした。

1. **誰が所有するか。** `unique_ptr` / `shared_ptr` / 参照 / 値。
   Java 版が `new` して返しているところは、ほぼ全章で `unique_ptr` を返す形になりました。
   共有が本当に要ったのは Flyweight と Observer くらいです
2. **仮想デストラクタを書いたか。** 純粋仮想関数を書いた章は全部これが必要でした。
   書き忘れは静かに漏れます
3. **値かコピーか参照か。** Java には無い判断です。`Object` を返していたところを
   `const T &` にするか `T` にするかで、性能も寿命も変わりました
4. **コンパイル時に決められないか。** テンプレートにすると vtable もヒープも消えます。
   Strategy・State・Visitor・Interpreter で、この選択肢が必ず出てきました

**新しい章に入ったら、この 4 つを順に聞いてください。** それで 8 割は片付きます。

### マイコンでの結論は、ほぼ全章で同じだった

- **ヒープを使わない**（確保するなら起動時に 1 回だけ）
- **例外を使わない**（エラーは戻り値。`bool` / エラーコード / 自前の Result 型）
- **実行時多態が本当に必要かを問う**（実装が 1 つなら仮想関数は不要）
- **コンパイル時に寄せる**（テンプレート、`constexpr`、固定長配列）

23 章分書いてみて、マイコン側の結論がここまで揃うとは思わなかったかもしれません。
逆に言えば、**マイコンでは GoF の多くがそのままの形では入らない**ということです。
「パターンを使わない」のではなく、**同じ意図を、確保も例外も無い形で実現する**のが仕事です。

### GoF の 23 個のうち、C++ で「自作する」ものは少ない

| パターン | C++ では |
| --- | --- |
| Iterator | `begin()` / `end()` と `<algorithm>` が既にある |
| Command | `std::function` がほぼそれ |
| Proxy | スマートポインタそのものが Proxy |
| Flyweight | `shared_ptr` と `string_view` |
| Visitor | `std::variant` + `std::visit` |
| Strategy | テンプレート引数 or `std::function` |
| Prototype | コピーコンストラクタが既にある（多態のときだけ `clone()`） |
| Singleton | 関数内 static（Meyers Singleton）。言語が保証する |

それでも 1 度ずつ手で実装したのは、**標準がなぜその形なのかを理解するため**です。
GoF 版の `Iterator` を書いてから `begin()` / `end()` を書くと、
「終端を別のイテレータで表す」設計が何を可能にしたのかが分かります。
自作を飛ばしていたら、`std::function` はただの便利な箱のままだったはずです。

### 次にやること

読み終わったら、**部内ライブラリの設計レビュー**をしてください。見るのは 2 点です。

1. **今のコードは、この 23 個のどれを使っているか。** 名前が付いていないだけで、
   実は Strategy だったり Observer だったりします。名前が付くとレビューが早くなります
2. **使うべきでないものを使っていないか。** 実装が 1 つしかない抽象、
   グローバル変数を改名しただけの Singleton、3 段になった生成、
   `unique_ptr` で書けるところの `shared_ptr`

そして [0. 使う前に](00_使う前に.md) の 4 つのチェックに戻ってください。

1. 今、実装は 2 つ以上あるか
2. この抽象を消したら、どの変更が難しくなるか
3. 誰がこのオブジェクトを解放するか、1 行で言えるか
4. 標準ライブラリに同じものが無いか

**23 章を読んだ今のほうが、あの 4 つは重く読めるはずです。**

---

前: [22. Command](22_Command.md) ／ 次: [この講習を終えて](README.md)
