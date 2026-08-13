# 14. Chain of Responsibility

> **結城本 第14章 対応。** `Support` / `NoSupport` / `LimitSupport` / `Trouble` を手元に開いてください。
>
> **この章のねらい**: 構造は Java 版とほとんど同じです。`resolve()` して、
> ダメなら `next` に回す。それだけです。
> **問題は `next` を何で持つか**の 1 点に集約されます。
> Java の `private Support next;` は GC が面倒を見てくれますが、
> C++ で `Support * next_;` と書くと、**next が先に死んだ瞬間に連鎖全体が地雷になります**。
> 生ポインタ・`unique_ptr`・「連鎖を作らない」の 3 択を並べて、
> 実務でどれを採るかまで決めます。

## 14.1 Java 版をそのまま C++ にすると

結城本の `Support` はこうです。

```java
public abstract class Support {
    private String name;
    private Support next;

    public Support setNext(Support next) {
        this.next = next;
        return next;
    }

    public final void support(Trouble trouble) {
        if (resolve(trouble)) {
            done(trouble);
        } else if (next != null) {
            next.support(trouble);
        } else {
            fail(trouble);
        }
    }

    protected abstract boolean resolve(Trouble trouble);
}
```

C++ に移すとこうなります。

```cpp
class FaultHandler
{
public:
  explicit FaultHandler(std::string name);
  virtual ~FaultHandler();

  FaultHandler(const FaultHandler &) = delete;
  FaultHandler & operator=(const FaultHandler &) = delete;

  FaultHandler & set_next(std::unique_ptr<FaultHandler> next);
  std::optional<FaultAction> support(const Fault & fault) const;

protected:
  virtual std::optional<FaultAction> resolve(const Fault & fault) const = 0;

private:
  std::string name_;
  std::unique_ptr<FaultHandler> next_;
};
```

変更点が 4 つあります。

### 変更点1: `virtual ~FaultHandler();` を足した

いつもの話です。`std::unique_ptr<FaultHandler>` で持つのだから、
仮想デストラクタが無いと派生のデストラクタが呼ばれません。
この章では**連鎖が丸ごと同じ経路で消える**ので、1 つ抜けているだけで
連鎖の途中からリークします。

### 変更点2: `Support next` を `std::unique_ptr<FaultHandler> next_` にした

**この章の本題です。** 14.3 で 3 択を全部並べます。

### 変更点3: コピーを禁止した

Java の `Support` はコピーという概念がありません。C++ では書かないと
コピーコンストラクタが**勝手に生えます**。連鎖のノードをコピーして何が起きてほしいのか、
答えられないなら禁止するのが正解です。

なお `next_` が `unique_ptr` である時点でコピーコンストラクタは暗黙 delete されますが、
**`= delete` と明示的に書いてください。** 「メンバの都合でたまたま禁止されている」のと
「設計として禁止した」のは別物で、後でメンバを差し替えたときに差が出ます。

### 変更点4: `support()` の戻りを `void` から `std::optional<FaultAction>` にした

Java 版は `done()` / `fail()` を呼んで終わりです。
C++ 版で「誰が処理したか」を戻り値で返す理由は 14.4 で書きます。

### 変わらない点: NVI

`support()` が `public` 非仮想、`resolve()` が `protected` 純粋仮想。
これは Java 版の `public final void support` / `protected abstract boolean resolve` と
**まったく同じ意図**です（第3章 Template Method の形）。

「ダメなら次に回す」というたらい回しのロジックを**派生に書かせない**のが肝で、
ここを派生に開放すると、`next` に回し忘れるハンドラが必ず 1 つ紛れ込みます。

## 14.2 誰が連鎖を所有するのか

Java 版の組み立てはこうです。

```java
Support alice = new NoSupport("Alice");
Support bob = new LimitSupport("Bob", 100);
alice.setNext(bob).setNext(charlie);
```

`alice` も `bob` も `charlie` も、参照が生きている限り GC が生かします。
**誰が誰を所有しているかは、どこにも書かれていません。** それで動くのが Java です。

C++ では書きます。`set_next()` が `unique_ptr` を受け取ることで、

> **各ハンドラは「自分の次」を所有する。連鎖全体の寿命は先頭が握る。**

が型に書かれました。呼ぶ側はこうなります。

```cpp
auto head = make_low_voltage_handler("low_voltage", 11000);
head->set_next(make_over_current_handler("over_current", 20000))
  .set_next(make_comm_timeout_handler("comm_timeout", 500));
// head を捨てると連鎖 3 段が全部消える
```

### `set_next()` が `*next_` を返す理由

Java 版の `setNext` は `return next;` です。C++ でも同じにします。

```cpp
FaultHandler & FaultHandler::set_next(std::unique_ptr<FaultHandler> next)
{
  next_ = std::move(next);
  return *next_;      // *this ではない
}
```

`next` は `std::move` した時点で空になるので、**`*next` を返してはいけません**。
`next_` に入れた**あと**の `*next_` を返します。

`*this` を返すとメソッドチェーンの意味が変わります。

```cpp
head->set_next(std::move(b)).set_next(std::move(c));
// *next_ を返す → head → b → c（正しい）
// *this  を返す → head → c（b が捨てられる）
```

`*this` 版が単に「順番が違う」で済まないのが C++ です。
2 回目の `set_next` で `head` の `next_` が上書きされ、**b はそこで解放されます**。
課題のテスト「連鎖の途中を差し替えると古い残りは破棄される」がこれを見ています。

## 14.3 C++ 固有の危険 — 連鎖の寿命

`next` の持ち方は 3 択です。**順に潰していきます。**

### (a) 生ポインタ + 「寿命は呼び出し側が保証する」規約

Java 版に一番近い形です。

```cpp
class Handler
{
public:
  void set_next(Handler * next) { next_ = next; }
  // ...
private:
  Handler * next_ = nullptr;
};
```

これを書くと、次のコードが**書けてしまいます**。

```cpp
int main()
{
  Handler head{"head"};
  {
    Handler tail{"tail"};      // ブロックを抜けると死ぬ
    head.set_next(&tail);
  }
  std::cout << head.support() << "\n";   // head は死んだ tail を指したまま
  return 0;
}
```

**コンパイルは通り、警告も出ません。** 実行すると死んだオブジェクトを触ります。
運が良ければ落ち、運が悪ければ「たまに変な値を返す」だけになります。
`-fsanitize=address` を付けて実行すれば報告されますが、
**サニタイザを掛けるまで気付けない**という時点で設計として弱いです。

Java なら `tail` への参照が `head` の中に残っているので GC は回収せず、落ちません。
**この差が、この章で C++ 側だけに追加される仕事です。**

生ポインタ版を選んでよいのは、**全ハンドラが静的記憶域にある**ときだけです
（14.7 のマイコン版がまさにそれ）。「呼び出し側が寿命を保証する」という規約を
コメントに書くだけでは、部活の共同開発では守られません。

### (b) `unique_ptr` で次を所有する

課題で実装するのがこれです。

```cpp
FaultHandler & set_next(std::unique_ptr<FaultHandler> next);
private:
  std::unique_ptr<FaultHandler> next_;
```

- 先頭が生きている限り連鎖全体が生きている
- 先頭を捨てれば連鎖全体が消える
- 「次が先に死ぬ」が**構造的に起こらない**

代償は 2 つあります。

1. **ヒープ確保がハンドラの数だけ走る**（マイコンで効く）
2. **ハンドラを他所と共有できない**。同じハンドラを 2 本の連鎖に入れることはできません

2 番目は制約に見えて、実は仕様の明確化です。共有したくなったら
`shared_ptr` ではなく、**そのハンドラをステートレスにして 2 個作る**方を先に検討してください。

### (c) 連鎖を作らない — `vector<unique_ptr<Handler>>` を順に回す

```cpp
std::vector<std::unique_ptr<FaultHandler>> handlers;
handlers.push_back(make_low_voltage_handler("low_voltage", 11000));
handlers.push_back(make_over_current_handler("over_current", 20000));

for (const auto & handler : handlers) {
  if (auto action = handler->support_alone(fault)) {
    return action;
  }
}
return std::nullopt;
```

**実務ではこれがいちばん素直です。** 理由は 4 つあります。

| 論点 | (b) 連鎖 | (c) 配列 |
| --- | --- | --- |
| 順番を見る | ソースを追って `set_next` を辿る | `push_back` の並びがそのまま順番 |
| 順番を変える | つなぎ替え。移動元が空になるので手順を間違えやすい | 要素を入れ替えるだけ |
| 寿命 | 先頭が全部を握る（暗黙的） | vector が全部を持つ（明示的） |
| 循環 | 作れてしまう形もある（14.5） | **構造的に不可能** |
| 深い連鎖 | 再帰。段数だけスタックを食う | ループ。一定 |

`Handler` から `next_` メンバが消えるのも大きい。
**ハンドラは「自分が処理できるか」だけを知っていればよく、
「次が誰か」を知る必要はありません。** (c) はその責務分離までやってくれます。

では GoF 版（連鎖）に意味が無いかというと、そうではありません。
**ハンドラごとに「次」が動的に変わる**とき、たとえば
「電圧異常のときだけ別系統に流す」ような分岐を持たせるなら、連鎖の形が要ります。
分岐が無いなら (c) です。

**課題では (b) と (c) を両方実装します。** 並べて書くと、(c) の単純さが手で分かります。

## 14.4 誰も処理しなかったとき — `bool` か `optional` か、そして例外を投げない理由

Java 版は `fail()` を呼んで終わり、`support()` は `void` です。
C++ での選択肢は 3 つです。

| 戻り値 | 分かること | 困ること |
| --- | --- | --- |
| `void` + 内部で `fail()` | 何も | 呼び出し側が「処理されたか」を判定できない |
| `bool` | 処理されたかどうか | **誰が**処理したか分からない。ログに出せない |
| `std::optional<FaultAction>` | 処理されたか + 誰が何をしたか | 型が少し重い |

この課題では `std::optional<FaultAction>` にしました。
異常処理の連鎖では「どのハンドラが何をしたか」がログの本体なので、
`bool` だと結局その情報を別経路で取りに行くことになります。

### 例外を投げない

「誰も処理できないのは異常事態だから `throw` すべきでは」と思ったはずです。**投げません。**

1. **マイコンでは `-fno-exceptions` が普通**です。`throw` を含むコードはそもそもビルドできません
2. 「誰も処理しなかった」は**想定内**です。例外は想定外のためのものです
3. 連鎖の途中から投げると、どこまで処理が進んだのか呼び出し側から分からなくなります

代わりに `std::nullopt` を返して、**呼び出し側に判断させます**。

```cpp
if (const auto action = chain->support(fault)) {
  logger.info(action->handler_name + ": " + action->action);
} else {
  logger.warn("unhandled fault");     // ここで初めて「どうするか」を決める
}
```

「未処理をどう扱うか」は連鎖の中ではなく**連鎖の外**で決めるべき、というのがこの形の主張です。
結城本の `fail()` は連鎖の中に置いていますが、C++ では外に出すほうが素直です。

なお `std::optional` 自体は `-fno-exceptions` でも使えますが、
`value()` は失敗時に `throw` します。`*action` か `has_value()` を使ってください。
マイコン向けの結論は 14.7 で別に出します。

## 14.5 無限ループの危険 — 連鎖が輪になる

連鎖が輪になると `support()` は永久に回ります。Java でも同じですが、
C++ では**スタックオーバーフローの症状が「なぜか再起動する」だけ**になりやすく、
原因に辿り着くのが遅れます。マイコンなら即ウォッチドッグリセットです。

生ポインタ版（(a)）なら、輪は簡単に作れます。

```cpp
a.set_next(&b);
b.set_next(&c);
c.set_next(&a);      // 輪になった
```

**組み立て直後に 1 回だけ検査**してください。Floyd の循環検出で足ります。

```cpp
bool has_cycle(const Node * head)
{
  const Node * slow = head;
  const Node * fast = head;
  while (fast != nullptr && fast->next() != nullptr) {
    slow = slow->next();
    fast = fast->next()->next();
    if (slow == fast) {
      return true;
    }
  }
  return false;
}
```

実行するとこうなります（`straight` が直線、`cyclic` が `c → a` を足した後）。

```
straight: 0
cyclic:   1
```

追加メモリはポインタ 2 本。組み立ては起動時に 1 回なので、コストは無視できます。

### `unique_ptr` 版ではどうか

**別々のノードによる輪は作れません。** 各ノードの所有者は 1 つだけで、
`c` の `next_` に `a` を入れるには `a` の `unique_ptr` を渡すしかなく、
その `a` はもう先頭ではなくなるからです。所有権が一意なら形は木にしかなりません。

ただし**自己所有**は書けてしまいます。

```cpp
auto head = std::make_unique<Handler>("head");
head->set_next(std::move(head));    // 自分を自分に所有させる
std::cout << "head is " << (head ? "alive" : "null") << "\n";
std::cout << "--- main を抜ける ---\n";
```

```
head is null
--- main を抜ける ---
```

**`dtor head` が出ません。** `head` は自分の `next_` に所有されているので、
誰もそれを解放しません。無限ループではなく**静かなリーク**になります。
`support()` を呼べばこちらは無限再帰します。

`set_next` の先頭に

```cpp
if (next.get() == this) {
  return *this;       // あるいは assert
}
```

を置いておけば潰せます。**検査するなら組み立て時。** 実行時に毎回検査してはいけません。

## 14.6 標準ライブラリ／言語機能に同じものが無いか

**クラスとしては無い**、が答えです。GoF の Chain of Responsibility に相当する
コンテナやユーティリティは標準ライブラリにありません。

ただし、**言語機能として 1 つあります。例外の `catch` です。**

```cpp
try {
  do_something();
} catch (const LowVoltage & e) {      // 処理できるか？
} catch (const OverCurrent & e) {     // できなければ次へ
} catch (...) {                       // 最後の砦
}
```

`catch` 節を上から順に試し、型が合うものが処理し、合わなければ次に回す。
さらに、どの `catch` も受けなければ**外側の `try` に伝播する**。
これは Chain of Responsibility そのものです。
しかも連鎖の組み立ては言語がやってくれるので、寿命の問題がありません。

なので判断はこうなります。

- **例外が使える環境（ROS 2 / PC）で、異常の伝播が目的**なら、まず `catch` の連鎖で足りないか考える
- **例外が使えない、または「処理した／しない」を値として扱いたい**なら、このパターンを自分で書く

マイコンは後者です。だからこの章は部活にとって意味があります。

`std::variant` + `std::visit` は「型で分岐」ですが、**たらい回しはしません**
（オーバーロード解決で 1 つに決まる）。用途が違います。

## 14.7 手元で試す

課題を解く前に、この 1 ファイルをコンパイルして**出力を予想してから**実行してください。
特に、最後の `head.reset()` で出てくる `dtor` の**順番**を当ててください。

```cpp
#include <iostream>
#include <memory>
#include <optional>
#include <string>

struct Fault
{
  int code;
};

class Handler
{
public:
  explicit Handler(std::string name, int mine)
  : name_(std::move(name)), mine_(mine)
  {
    std::cout << "ctor " << name_ << "\n";
  }

  virtual ~Handler() { std::cout << "dtor " << name_ << "\n"; }

  Handler & set_next(std::unique_ptr<Handler> next)
  {
    next_ = std::move(next);
    return *next_;
  }

  std::optional<std::string> support(const Fault & fault) const
  {
    if (fault.code == mine_) {
      return name_;
    }
    if (next_ != nullptr) {
      return next_->support(fault);
    }
    return std::nullopt;
  }

private:
  std::string name_;
  int mine_;
  std::unique_ptr<Handler> next_;
};

int main()
{
  auto head = std::make_unique<Handler>("low_voltage", 1);
  head->set_next(std::make_unique<Handler>("over_current", 2))
    .set_next(std::make_unique<Handler>("comm_timeout", 3));

  const auto hit = head->support(Fault{3});
  std::cout << "hit=" << (hit ? *hit : std::string{"(none)"}) << "\n";

  const auto miss = head->support(Fault{9});
  std::cout << "miss=" << (miss ? *miss : std::string{"(none)"}) << "\n";

  std::cout << "--- head を捨てる ---\n";
  head.reset();
  std::cout << "--- 捨てたあと ---\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: <code>dtor</code> は先頭からか、末尾からか</summary>

```
ctor low_voltage
ctor over_current
ctor comm_timeout
hit=comm_timeout
miss=(none)
--- head を捨てる ---
dtor low_voltage
dtor over_current
dtor comm_timeout
--- 捨てたあと ---
```

**先頭からです。** `~Handler()` の本体（`dtor` の出力）が走ったあとに、
メンバである `next_` が破棄されます。メンバの破棄はデストラクタ本体の**後**なので、
`low_voltage` → `over_current` → `comm_timeout` の順になります。

「末尾から」と予想した人は、リストの解放を再帰で書いたときの感覚だと思います。
デストラクタ本体とメンバ破棄の順番を思い出してください。

もう 1 つ。`set_next` の `return *next_;` を `return *this;` に変えると、こうなります。

```
ctor low_voltage
ctor over_current
ctor comm_timeout
dtor over_current
hit=comm_timeout
miss=(none)
--- head を捨てる ---
dtor low_voltage
dtor comm_timeout
--- 捨てたあと ---
```

**組み立ての途中で `dtor over_current` が出ています。** 2 回目の `set_next` で
`head` の `next_` が `comm_timeout` に上書きされ、`over_current` はそこで解放されました。
連鎖は 3 段のつもりが 2 段です。しかも `hit=comm_timeout` は変わらないので、
**テストの書き方によっては気付けません**。
メソッドチェーンが何を返しているかは、寿命に直結します。

なお、この連鎖は**深さの分だけ再帰**します。ハンドラが 5 段程度なら問題ありませんが、
数百段になるなら 14.3 の (c) でループにしてください。
</details>

## 14.8 マイコンでの結論

**`unique_ptr` の連鎖は使いません。** 理由は 2 つです。

1. ハンドラの数だけヒープ確保が走る。起動時 1 回とはいえ、断片化の種を増やす意味がない
2. `std::string` / `std::optional` / `std::function` が芋づるで付いてくる

採るのは 14.3 の **(c) の静的版**です。
**ハンドラは静的記憶域に置き、`next` を持たせず、固定長配列の並びで順番を表します。**
確保はゼロ、連鎖の寿命という問題そのものが消えます。

```cpp
#include <cstddef>
#include <cstdio>

enum class FaultKind
{
  kLowVoltage,
  kOverCurrent,
  kCommTimeout,
};

struct Fault
{
  FaultKind kind;
  int magnitude;
};

/// std::optional は使わない（value() が throw しうる）。
/// handled == false のとき handler_name / action は読まない約束。
struct FaultAction
{
  bool handled = false;
  const char * handler_name = nullptr;
  const char * action = nullptr;
};

/// next を持たない。連鎖の形は配列側が持つ。
class FaultHandler
{
public:
  virtual ~FaultHandler() = default;
  virtual FaultAction resolve(const Fault & fault) const = 0;
};

class LowVoltageHandler : public FaultHandler
{
public:
  constexpr explicit LowVoltageHandler(int threshold_mv)
  : threshold_mv_(threshold_mv)
  {
  }

  FaultAction resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kLowVoltage && fault.magnitude < threshold_mv_) {
      return FaultAction{true, "low_voltage", "reduce_duty"};
    }
    return FaultAction{};
  }

private:
  int threshold_mv_;
};

class OverCurrentHandler : public FaultHandler
{
public:
  constexpr explicit OverCurrentHandler(int limit_ma)
  : limit_ma_(limit_ma)
  {
  }

  FaultAction resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kOverCurrent && fault.magnitude >= limit_ma_) {
      return FaultAction{true, "over_current", "cut_output"};
    }
    return FaultAction{};
  }

private:
  int limit_ma_;
};

// 静的記憶域。ヒープも new も使わない。
LowVoltageHandler low_voltage{11000};
OverCurrentHandler over_current{20000};

// 連鎖の「順番」は、この配列の並びそのもの。
FaultHandler * const kChain[] = {&low_voltage, &over_current};
constexpr std::size_t kChainSize = sizeof(kChain) / sizeof(kChain[0]);

FaultAction dispatch(const Fault & fault)
{
  for (std::size_t i = 0; i < kChainSize; ++i) {
    const FaultAction action = kChain[i]->resolve(fault);
    if (action.handled) {
      return action;
    }
  }
  return FaultAction{};   // 誰も処理しなかった
}
```

`main` で 3 件流した結果です。

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti mcu.cpp -o mcu && ./mcu
```

```
low_voltage -> reduce_duty
over_current -> cut_output
unhandled
```

ポイントを 4 つ。

- **`std::optional` を使わず、`bool` を持つ POD にした。** `-fno-exceptions` 下で
  `value()` を誤って呼ぶ事故を型ごと消しています
- **`std::string` を使わず `const char *`。** 名前はリテラルなので確保が要りません
- **ハンドラは名前空間スコープの変数。** 動的確保も `next` のつなぎ替えもありません。
  ただし**静的初期化順序**には注意（第5章 Singleton の話。他の静的オブジェクトに
  依存させないこと）
- **`kChain` は `FaultHandler * const []`。** 順番はコンパイル時に決まり、
  実行時に壊れません

仮想関数はハンドラ数だけ vtable が要りますが、
`resolve` 1 個ずつなので数十バイト規模です。ここは払ってよいコストです。
どうしても払いたくない、かつハンドラがコンパイル時に確定しているなら、
継承をやめて関数ポインタの配列にしてください。

```cpp
using ResolveFn = FaultAction (*)(const Fault &);
constexpr ResolveFn kChain[] = {&resolve_low_voltage, &resolve_over_current};
```

vtable はゼロになりますが、ハンドラが状態（閾値）を持てなくなるので、
閾値をグローバル定数にするか引数で渡すことになります。**まずは仮想関数版で書いてください。**

## 14.9 ROS 2 での結論（補足）

rclcpp に GoF 版の Chain of Responsibility クラスは出てきません。
似た構造は 2 箇所にあります。

- **パラメータのコールバック**: `add_on_set_parameters_callback()` で登録した検証関数が
  順に呼ばれ、1 つでも拒否すれば全体が拒否されます。「順に試す」形ですが、
  最初に成功したもので打ち切る CoR とは終了条件が逆（全部を通す）です
- **ライフサイクルノードの遷移コールバック**: 状態ごとに担当が分かれます

自作するなら、ROS 2 側では例外も `std::function` も使えるので
`std::vector<std::function<std::optional<FaultAction>(const Fault &)>>` で足ります。
**クラス階層を作る必要はほぼありません。**

```cpp
std::vector<std::function<std::optional<FaultAction>(const Fault &)>> handlers;
handlers.push_back([](const Fault & f) -> std::optional<FaultAction> {
  if (f.kind == FaultKind::kLowVoltage && f.magnitude < 11000) {
    return FaultAction{"low_voltage", "reduce_duty"};
  }
  return std::nullopt;
});
```

ハンドラが状態を持たず、1 つあたり数行なら、これが最短です。
継承を持ち出すのは、ハンドラが**状態を持つ**か、**単体でテストしたい**ときだけにしてください。

## 14.10 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `a.set_next(b).set_next(c)` で b が消えている | `set_next` が `*next_` ではなく `*this` を返している |
| `set_next` の中で `*next` を返そうとして落ちる | `std::move(next)` の後の `next` は空。`*next_` を返す |
| 連鎖を作った直後にプログラムが固まる | 輪になっている。14.5 の検査を組み立て時に入れる |
| 先頭を捨てても 2 段目以降が解放されない | `next_` が生ポインタ。`unique_ptr` にする |
| 2 段目のデストラクタが呼ばれない | 基底の仮想デストラクタが無い |
| ハンドラを 2 本の連鎖に入れようとしてコンパイルできない | `unique_ptr` は共有できない。ハンドラを 2 個作る |
| 派生の `resolve()` の中で `next_->support()` を呼びたくなった | NVI が壊れています。たらい回しは `support()` の仕事 |
| `-fno-exceptions` で `optional::value()` がリンクできない | `*opt` か `has_value()` を使う |
| ハンドラを増やしたらスタックが足りなくなった | 連鎖は再帰。段数が多いなら (c) のループへ |

## 14.11 対応する課題

```bash
./drill run dp14
```

`exercises/dp14_chain_of_responsibility/src/fault_chain.cpp` に、
ロボットの異常検知（電圧低下・過電流・通信断）の処理系を実装します。

1. **`FaultHandler::~FaultHandler()`** — 破棄ログに自分の名前を残す
2. **`FaultHandler::set_next()`** — `unique_ptr` で次を所有し、**次への参照**を返す
3. **`FaultHandler::support()`** — 自分 → 次 → …。誰も処理しなければ `std::nullopt`
4. **`FaultHandler::support_alone()`** — 次には回さず自分だけに聞く
5. **`dispatch()`** — 14.3 (c) の配列方式
6. **3 つの具体ハンドラの `resolve()`**

テストが見るのは、

- 適切なハンドラが処理し、それ以外は素通しすること
- **連鎖の順番を変えると処理するハンドラが変わる**こと
- 誰も処理しなかった場合が `std::nullopt` になること
- **先頭を破棄すると連鎖全体が破棄される**こと（デストラクタのログの並びで検証）
- `set_next()` が `*this` ではなく次のハンドラの参照を返していること（アドレス比較）
- 連鎖版と配列版で答えが一致すること

## 14.12 この章のまとめ

- 構造は Java 版とほぼ同じ。**変わるのは `next` を何で持つか**の 1 点
- 生ポインタ版は「次が先に死ぬ」を止められない。**Java には無い仕事**
- `unique_ptr` で次を所有すると、**先頭の寿命が連鎖全体の寿命**になる
- `set_next()` は `*this` ではなく **`*next_`** を返す。間違えると 2 段目が黙って解放される
- 誰も処理しなかったら **`std::nullopt`**。例外は投げない（`-fno-exceptions` のため）
- 分岐が要らないなら、**連鎖を作らず `vector` を順に回す (c) が実務では最も素直**
- 輪になると止まらない。**検査は組み立て時に 1 回**。`unique_ptr` なら別ノード同士の輪は作れない
- 標準ライブラリにクラスは無いが、**例外の `catch` の連鎖が言語機能版**
- マイコンでは静的ハンドラ + 固定長配列。**確保ゼロ、寿命問題ゼロ**

---

前: [13. Visitor](13_Visitor.md) ／ 次: 15. Facade（準備中）
