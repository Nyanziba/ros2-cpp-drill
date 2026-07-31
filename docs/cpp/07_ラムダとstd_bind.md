# 7. ラムダと `std::bind`

> **この章のねらい**: 課題 01 でこの 2 行を書きます。
>
> ```cpp
> timer_ = this->create_wall_timer(
>   500ms, std::bind(&MinimalPublisher::timer_callback, this));
> ```
>
> `&` が付いた関数名、`this`、そして課題 02 では `_1` という謎の変数が出てきます。
> 「メンバ関数を関数として渡す」という、C++ でいちばん記法が独特な部分です。
> ドリルにはラムダが 56 箇所、`std::bind` が 22 箇所、`placeholders` が 18 箇所あります。
> **どちらも ROS 2 の規約で認められている**ので、両方読めるようにします。

## 7.1 関数ポインタでは足りない

C にも「関数を渡す」機能はあります。

```cpp
#include <iostream>

void greet() { std::cout << "hello\n"; }

void call_it(void (*f)()) { f(); }

int main()
{
  call_it(greet);
  return 0;
}
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/sK3Gc3PaY)

これで足りるなら `std::bind` もラムダも要りません。足りないのは、**状態を持てない**からです。

`MinimalPublisher::timer_callback` は `this` を必要とします。
`count_` や `publisher_` に触るからです。しかし関数ポインタには「どのオブジェクトのか」を
入れる場所がありません。

```cpp
void (*f)() = &MinimalPublisher::timer_callback;   // エラー
```

```
error: cannot convert ‘void (MinimalPublisher::*)()’ to ‘void (*)()’
```

型が違います。`void (MinimalPublisher::*)()` は**メンバ関数ポインタ**という別の型で、
「呼ぶにはオブジェクトが必要」という情報が型に入っています。

```cpp
auto f = &MinimalPublisher::timer_callback;
MinimalPublisher obj;
(obj.*f)();          // オブジェクトを明示して呼ぶ。記法が独特
```

**「関数」と「対象のオブジェクト」を 1 つに束ねたものが欲しい。**
これを解決するのが `std::bind` とラムダです。

## 7.2 `std::bind` — 引数を先に埋めておく

`std::bind` は「関数と、その引数の一部を、あらかじめ束ねる」道具です。

```cpp
#include <functional>
#include <iostream>

int add(int a, int b) { return a + b; }

int main()
{
  auto add10 = std::bind(add, 10, std::placeholders::_1);
  std::cout << add10(5) << "\n";     // 15
  std::cout << add10(7) << "\n";     // 17
  return 0;
}
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/EYEzThcoY)

```
15
17
```

`std::bind(add, 10, _1)` は「`add` の 1 番目の引数は 10 で固定、
2 番目は呼び出し時にもらう」という意味です。

**`_1` は「呼び出し時の 1 番目の引数をここに入れる」というプレースホルダです。**
`std::placeholders` 名前空間にあります。長いので `using` するのが慣習です。

```cpp
using std::placeholders::_1;
```

課題 02 がまさにこの形です。

```cpp
// solutions/02_subscriber/src/minimal_subscriber.cpp
using std::placeholders::_1;

MinimalSubscriber::MinimalSubscriber()
: Node("minimal_subscriber")
{
  subscription_ = this->create_subscription<std_msgs::msg::String>(
    "topic", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));
}
```

### メンバ関数を bind する

3 つの部品があります。

```cpp
std::bind(&MinimalSubscriber::topic_callback, this, _1)
//        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  ^^^^  ^^
//        ① メンバ関数ポインタ                ② 対象  ③ 引数の受け口
```

**① `&クラス名::関数名`** — `&` が必要です。忘れるとエラーになります。

```cpp
std::bind(MinimalSubscriber::topic_callback, this, _1)   // & を忘れた
```

```
error: invalid use of non-static member function
```

**② `this`** — 「どのオブジェクトのメンバ関数か」。
メンバ関数の暗黙の第 0 引数が `this` なので、それを埋めています。
`std::bind` から見ると `this` は「1 番目の引数」の扱いです。

**③ `_1`** — メッセージが入る場所。
`topic_callback(const std_msgs::msg::String & msg)` は引数が 1 つなので `_1` が 1 つ。

引数が 0 個のコールバック（タイマー）では `_1` が要りません。

```cpp
// 課題 01: timer_callback() は引数なし
std::bind(&MinimalPublisher::timer_callback, this)
```

引数が 2 つのコールバック（サービス）では `_1, _2` になります。

```cpp
// 課題 04
std::bind(&AddTwoIntsServer::add, this, _1, _2)
```

**`_N` の数は、呼ばれるときに渡される引数の数と一致させてください。**
これが `std::bind` でいちばん間違えやすい点です。

### `std::bind` は引数の順番も入れ替えられる

使う場面はほぼありませんが、`_1` の意味を理解する助けになります。

```cpp
auto swapped = std::bind(subtract, _2, _1);
swapped(3, 10);      // subtract(10, 3) が呼ばれる
```

`_1` は「呼び出し時の 1 番目」なので、書く位置は自由です。

## 7.3 ラムダ式

同じことをラムダで書きます。

```cpp
timer_ = this->create_wall_timer(500ms, [this]() { this->timer_callback(); });
```

構文を分解します。

```cpp
[this](const std_msgs::msg::String & msg) { this->topic_callback(msg); }
//^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//  ①            ②                                    ③
```

| 部分 | 名前 | 意味 |
| --- | --- | --- |
| `[this]` | **キャプチャ** | 外側のどの変数を使えるようにするか |
| `(...)` | 引数リスト | ふつうの関数と同じ |
| `{ ... }` | 本体 | ふつうの関数と同じ |

戻り値の型は推論されます。明示したいときは `-> 型` を挟みます。

```cpp
[](int x) -> double { return x / 2.0; }
```

### キャプチャの種類

ここがラムダの本質です。

```cpp
#include <iostream>

int main()
{
  int a = 1;
  int b = 2;

  auto by_copy   = [a]()      { std::cout << "copy   a=" << a << "\n"; };
  auto by_ref    = [&a]()     { std::cout << "ref    a=" << a << "\n"; };
  auto all_copy  = [=]()      { std::cout << "all=   a=" << a << " b=" << b << "\n"; };
  auto all_ref   = [&]()      { std::cout << "all&   a=" << a << " b=" << b << "\n"; };
  auto none      = []()       { std::cout << "none\n"; };

  a = 99;
  b = 99;

  by_copy();
  by_ref();
  all_copy();
  all_ref();
  none();
  return 0;
}
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/13rareMGx)

```
copy   a=1
ref    a=99
all=   a=1 b=2
all&   a=99 b=99
none
```

**`[a]`（コピー）は、ラムダを作った時点の値を覚えています。**
そのあと `a` を 99 にしても、ラムダの中では 1 のままです。

**`[&a]`（参照）は、現在の `a` を見ます。** だから 99 になります。

| キャプチャ | 意味 | 危険度 |
| --- | --- | --- |
| `[]` | 何も使わない | 安全 |
| `[a]` | `a` をコピー | 安全 |
| `[&a]` | `a` を参照 | **`a` の寿命に注意** |
| `[=]` | 使うものを全部コピー | 安全だが `this` は例外（後述） |
| `[&]` | 使うものを全部参照 | **危険** |
| `[this]` | `this` ポインタをコピー | **オブジェクトの寿命に注意** |
| `[a = expr]` | 初期化キャプチャ（C++14） | 安全 |

### `[&]` と `[this]` の危険

参照キャプチャしたラムダが、参照先より長生きすると壊れます。

```cpp
std::function<void()> make_bad()
{
  int local = 42;
  return [&local]() { std::cout << local; };   // local はこの関数を抜けると消える
}

auto f = make_bad();
f();                                            // 未定義動作
```

4 章のダングリング参照と同じ問題が、ラムダの中で起きています。

**`[this]` はもっと注意が必要です。** `this` はポインタなので、
コピーキャプチャしても「ポインタをコピー」しただけです。オブジェクトは共有されます。

```cpp
class Node
{
public:
  void start()
  {
    timer_ = create_wall_timer(500ms, [this]() { tick(); });
  }
private:
  void tick() { ++count_; }
  int count_ = 0;
};
```

**ノードが破棄されたあとにラムダが呼ばれると、死んだ `this` を触ります。**

rclcpp では実害が出にくくなっています。6 章のとおり、
タイマーは `timer_` メンバが所有していて、ノードが死ぬと `timer_` も死ぬからです。
**「タイマーがノードより長生きすることはない」ので `[this]` が安全に使えます。**
これは rclcpp の所有権モデルに支えられた安全性で、一般的な保証ではありません。

**`[=]` は `this` をコピーしません。** ポインタをコピーするだけです。
C++20 では `[=, this]` と明示することが推奨され、暗黙の `this` キャプチャは非推奨になりました。
`[this]` と明示的に書くのが今の作法です。

### 初期化キャプチャ — `unique_ptr` を渡す

ムーブしたいものをキャプチャするときに使います。

```cpp
auto p = std::make_unique<std_msgs::msg::String>();
auto f = [msg = std::move(p)]() { std::cout << msg->data; };
```

`[p]` ではコピーになって `unique_ptr` なのでエラーです。
`[msg = std::move(p)]` なら、ラムダの中に所有権を移せます（5 章）。

### `mutable`

コピーキャプチャした変数は、既定では書き換えられません。

```cpp
int count = 0;
auto f = [count]() { ++count; };    // エラー
auto g = [count]() mutable { ++count; };   // OK（ラムダの中のコピーが増える）
```

`mutable` を付けたラムダは「呼ぶたびに自分の中のコピーが変わる」ので、
状態を持つ関数オブジェクトになります。

## 7.4 `std::function` — 何でも入る箱

ラムダの型は**名前がありません。** コンパイラが生成する匿名の型です。

```cpp
auto f = [](int x) { return x * 2; };   // f の型は「そのラムダの型」
```

だから変数に入れるには `auto` を使うしかありません。
**メンバ変数として持ちたい**、あるいは**関数の引数として受けたい**場合はこれでは困ります。

`std::function<戻り値(引数...)>` が受け皿になります。

```cpp
#include <functional>
#include <iostream>

void run_twice(const std::function<void(int)> & f)
{
  f(1);
  f(2);
}

int main()
{
  int total = 0;
  run_twice([&total](int x) { total += x; std::cout << "  x=" << x << "\n"; });
  std::cout << "total=" << total << "\n";

  std::function<void(int)> stored = [](int x) { std::cout << "  stored " << x << "\n"; };
  run_twice(stored);
  return 0;
}
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/c3e6E69d8)

```
  x=1
  x=2
total=3
  stored 1
  stored 2
```

`std::function` にはラムダも `std::bind` の結果も関数ポインタも入ります。
`create_subscription` が両方受け取れるのはこのおかげです。

代わりに実行時のコストがあります。中身を型消去してヒープに置くことがあり、
呼び出しが間接になってインライン展開されません。
**テンプレートで受けられる場所では `auto` / テンプレートのほうが速い**のですが、
1 制御周期に数回のコールバックでは無視できます。

## 7.5 ラムダと `std::bind` はどちらを使うべきか

**ROS 2 の公式規約は、どちらにも優劣を付けていません。**

[コーディング規約](../ros2-コーディング規約.md)の「言語機能の可否」の表を見てください。

| 機能 | ROS 2 の立場 |
| --- | --- |
| **ラムダ / `std::function` / `std::bind`** | **制限なし**（"No restrictions"） |

**「`bind` は古いから直すべき」は公式ルールではありません。**
これは覚えておいてください。既存コードを見て「直さなければ」と思う必要はありません。

実務的な比較を挙げます。

| 観点 | `std::bind` | ラムダ |
| --- | --- | --- |
| 公式チュートリアルの字面 | **こちら** | 一部にある |
| 引数の数が変わったとき | `_1` の数を直す必要がある | 引数リストを直す（コンパイラが教えてくれる） |
| 引数を無視したいとき | 書ける | 書ける |
| 一部の引数だけ渡す | **得意** | 手で書く |
| コンパイラの最適化 | されにくい | **されやすい** |
| エラーメッセージ | **非常に読みにくい** | 読みやすい |
| デバッガでのステップ実行 | 追いにくい | 追いやすい |

**エラーメッセージの差は実際に大きいです。**
`std::bind` は `_1` の数を間違えると、テンプレートの深いところから数十行のエラーが出ます。
ラムダなら「引数の数が合わない」と 1 行で出ます。

このドリルは**公式チュートリアルの字面に合わせることを優先**して `std::bind` を使っています。
公式ドキュメントを開きながら進められることのほうが、学習段階では価値が高いという判断です。

自分のコードでは、**新規に書くならラムダ**を勧めます。

```cpp
// std::bind 版（公式・このドリル）
subscription_ = this->create_subscription<std_msgs::msg::String>(
  "topic", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));

// ラムダ版（同じ動作）
subscription_ = this->create_subscription<std_msgs::msg::String>(
  "topic", 10,
  [this](const std_msgs::msg::String & msg) { this->topic_callback(msg); });
```

ラムダ版のほうが 1 行長いですが、**引数の型が書いてある**のが利点です。
「このコールバックは何を受け取るのか」がその場で読めます。

## 手元で試す

**キャプチャの違いを、寿命の事故ごと体験します。**

```cpp
// lambda.cpp
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Registry
{
public:
  void on_event(std::function<void(int)> f) { handlers_.push_back(std::move(f)); }
  void fire(int value)
  {
    for (const auto & h : handlers_) {
      h(value);
    }
  }
private:
  std::vector<std::function<void(int)>> handlers_;
};

class Counter
{
public:
  explicit Counter(std::string name) : name_(std::move(name)) {}

  void subscribe(Registry & r)
  {
    r.on_event([this](int v) { add(v); });   // this をキャプチャ
  }

  void add(int v) { total_ += v; }
  void show() const { std::cout << "  " << name_ << " total=" << total_ << "\n"; }

private:
  std::string name_;
  int total_ = 0;
};

int main()
{
  Registry reg;

  // ① 値キャプチャと参照キャプチャ
  int base = 10;
  reg.on_event([base](int v) { std::cout << "  copy: base+v = " << base + v << "\n"; });
  reg.on_event([&base](int v) { std::cout << "  ref:  base+v = " << base + v << "\n"; });
  base = 1000;

  // ② this キャプチャ
  Counter c("alive");
  c.subscribe(reg);

  std::cout << "fire(5):\n";
  reg.fire(5);
  c.show();

  // ③ std::bind との等価性
  std::cout << "bind:\n";
  Counter d("bound");
  reg.on_event(std::bind(&Counter::add, &d, std::placeholders::_1));
  reg.fire(3);
  d.show();

  return 0;
}
```

**予想を 3 つ立ててください。**

1. `copy:` の行は `15` と出るか `1005` と出るか
2. `ref:` の行はどちらか
3. 最後の `d total=` はいくつか（`fire(3)` は 1 回だけだが、`fire(5)` の分は入るか）

```bash
g++ -std=c++17 -Wall -Wextra lambda.cpp -o lambda && ./lambda
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/E93xYx5W5)

次に**寿命の事故を起こします。** `main` の `②` の部分を次に差し替えてください。

```cpp
  // ② this キャプチャ（危険版）
  {
    Counter tmp("dead");
    tmp.subscribe(reg);
  }   // tmp はここで死ぬ。しかし reg はまだ tmp の this を持っている
```

```bash
g++ -std=c++17 -Wall -Wextra -fsanitize=address lambda.cpp -o lambda && ./lambda
```

**`-fsanitize=address` を付けてください。** 付けないと「たまたま動く」ことがあります。
`stack-use-after-scope` として報告されます。

**これがラムダでいちばん多い事故です。**
`[this]` は「オブジェクトが生きている限り安全」で、
**その保証をするのはラムダを登録した側の責任**です。

rclcpp では 6 章の所有権モデルがこれを担保します。
タイマーはノードのメンバなので、ノードより長生きしません。
だからドリルのコードで `[this]` は安全です。

最後に `std::bind` のエラーメッセージを見ておきます。
`_1` を 1 つ増やしてください。

```cpp
reg.on_event(std::bind(&Counter::add, &d, std::placeholders::_1, std::placeholders::_2));
```

**エラーの行数を数えてください。**

```bash
$ g++ -std=c++17 -c lambda.cpp -o /dev/null 2>&1 | wc -l
22
```

**22 行**出ました。しかもそのほとんどが `std::_Bind<...>` のテンプレート展開で、
「`_2` が 1 つ多い」とはどこにも書かれていません。
ラムダで引数の数を間違えれば `too few arguments to function` の 1〜2 行で済みます。
これが「新規ならラムダ」を勧める実務的な理由です。

## つまずきポイント

**`error: invalid use of non-static member function`**
`std::bind` に渡すメンバ関数の `&` を忘れています。
`&MinimalSubscriber::topic_callback` と書いてください。

**`std::bind` で数十行のエラーが出た**
まず `_N` の数を確認してください。9 割これです。

- タイマーのコールバック（引数 0）→ `_1` は不要
- 購読のコールバック（引数 1）→ `_1`
- サービスのコールバック（引数 2）→ `_1, _2`

エラーの中から `no match for call to (std::_Bind<...>)` の行を探すと、
実際に何個渡されているかが読めます。

**`error: ‘_1’ was not declared in this scope`**
`using std::placeholders::_1;` を書き忘れています。
または `#include <functional>` が漏れています。

**ラムダの中でメンバ変数が見えない**
`[this]` か `[=]` をキャプチャに書いてください。
`[]` では外側の何も見えません。

**`error: assignment of read-only variable`（ラムダの中で）**
コピーキャプチャした変数を書き換えています。`mutable` を付けるか、参照キャプチャにします。

**キャプチャした `unique_ptr` でエラー**
`[p]` はコピーになるので `unique_ptr` では通りません。
`[p = std::move(p)]` と初期化キャプチャを使ってください。

**ラムダを `std::function` に入れたら遅くなった**
型消去のコストです。テンプレートで受けられる場所なら `auto` のままにしてください。
`create_subscription` のような「ライブラリ側が `std::function` を要求している」場所では
避けられません（そして問題になりません）。

## ドリルのどこで出るか

| 課題 | この章の何が出るか |
| --- | --- |
| 01 | `std::bind(&MinimalPublisher::timer_callback, this)` — `_1` なし |
| 02 | `std::bind(&MinimalSubscriber::topic_callback, this, _1)` — `_1` 1 つ |
| 04 | サービスのコールバックは `_1, _2`（request と response） |
| 05 | `async_send_request` の完了処理をラムダで書く場面がある |
| 07 | `ParameterEventHandler` にラムダを登録する |
| 10 | アクションの `handle_goal` / `handle_cancel` / `handle_accepted` の 3 つを登録 |
| 13 | コールバックグループを分けた購読とクライアント |
| テスト | `tools/drill_harness.hpp` の中でラムダを多用（56 箇所の大半） |

課題 10 がこの章のいちばん重い応用です。**3 つのコールバックを同時に登録します。**

```cpp
using namespace std::placeholders;

action_server_ = rclcpp_action::create_server<Fibonacci>(
  this,
  "fibonacci",
  std::bind(&FibonacciActionServer::handle_goal, this, _1, _2),
  std::bind(&FibonacciActionServer::handle_cancel, this, _1),
  std::bind(&FibonacciActionServer::handle_accepted, this, _1));
```

**`_1, _2` / `_1` / `_1` と数が違います。** ここを間違えるのが課題 10 で最も多いつまずきです。
数が合っているかは、各ハンドラの宣言を見て確認してください。

そして `handle_accepted` の中では、
**ブロックせずに別スレッドへ逃がす**というアクションサーバ特有の作法が必要になります。

```cpp
void handle_accepted(const std::shared_ptr<GoalHandleFibonacci> goal_handle)
{
  std::thread{std::bind(&FibonacciActionServer::execute, this, _1), goal_handle}.detach();
}
```

これは 12 章（並行性）の話につながります。

## 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `cpp07_lambda` — ラムダとコールバックを登録する

```bash
./drill run cpp07
```

課題側からは `./drill read` でこの章に戻ってこられます。

## 参考

- [ROS 2 のコーディング規約](../ros2-コーディング規約.md) — ラムダ / `std::function` / `std::bind` はいずれも "No restrictions"
- `.claude/skills/rclcpp-refactor/SKILL.md` — 既存コードで bind とラムダのどちらを選ぶかの判断材料
- `cppreference` の [Lambda expressions](https://en.cppreference.com/w/cpp/language/lambda) と [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind)

---

前章 → [6. スマートポインタ](06_スマートポインタ.md)
次章 → [8. `auto` と型推論](08_autoと型推論.md)
