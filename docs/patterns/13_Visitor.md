# 13. Visitor

> **結城本 第13章 対応。** `Visitor` / `Element` / `File` / `Directory` / `ListVisitor` を手元に開いてください。
>
> **この章のねらい**: `accept()` が `visitor.visit(this)` を呼ぶ、あの回りくどい構造。
> **なぜ 1 回で済まないのか**を、C++ のオーバーロード解決の規則から説明します。
> そのうえで、C++17 には**継承も仮想関数も `accept` も要らない別解**があります。
> `std::variant` + `std::visit` です。GoF 版と variant 版を両方実装して、
> **同じ木に対して同じ結果が出ること**を確かめます。
> 23 章の中で、**Java 版と C++ 版の見た目が最も離れる章**です。

## 13.1 Java 版をそのまま C++ にすると

結城本の `Visitor` と `Element` はこうです。

```java
public abstract class Visitor {
    public abstract void visit(File file);
    public abstract void visit(Directory directory);
}

public interface Element {
    public abstract void accept(Visitor v);
}
```

C++ に移すとこうなります。

```cpp
class SensorCheck;
class MotorCheck;
class CheckGroup;

class DiagVisitor
{
public:
  virtual ~DiagVisitor() = default;                    // 変更点1
  virtual void visit(const SensorCheck & node) = 0;    // 変更点2
  virtual void visit(const MotorCheck & node) = 0;
  virtual void visit(const CheckGroup & node) = 0;
};

class DiagNode
{
public:
  virtual ~DiagNode() = default;
  virtual void accept(DiagVisitor & visitor) const = 0;  // 変更点3
};
```

### 変更点1: 仮想デストラクタ

第1章と同じです。`virtual` を 1 つでも書いたら仮想デストラクタも書きます。
**`DiagVisitor` にも `DiagNode` にも要ります。** 訪問者も基底ポインタで持ち回るからです。

### 変更点2: 引数は `const Derived &`。値でも基底型でもない

Java の `visit(File file)` は参照渡しです。C++ で

```cpp
virtual void visit(SensorCheck node) = 0;    // 値渡し。毎回コピー
```

と書くと、**訪問のたびに要素がまるごとコピーされます。**
さらに悪いことに、要素側が `CheckGroup`（`unique_ptr` の `vector` を持つ）だと
コピーコンストラクタが無いのでそもそもコンパイルが通りません。

もっと危ないのは基底型で受けることです。

```cpp
virtual void visit(DiagNode node) = 0;       // スライシング
```

派生部分が切り落とされます。Java には**そもそもスライシングが存在しない**ので、
本を写経しているだけでは気づけません。

`const` を付けるのは、訪問者は普通は要素を読むだけだからです。
書き換える訪問者を作るときだけ `const` を外します（そのときは `accept` の `const` も外れます）。

### 変更点3: `accept` は `const` メンバ関数にする

要素を変更しない訪問なら `accept` も `const` です。
Java にはこの区別が無いので、**書き忘れると `const DiagNode &` を訪問できません。**

## 13.2 なぜ `accept` が要るのか — オーバーロードは静的型で決まる

ここがこの章の本題です。「`visit` のオーバーロードがあるなら、
訪問者に要素を渡すだけでいいのでは」と思ったはずです。試すとこうなります。

```cpp
struct Node { virtual ~Node() = default; };
struct Sensor : Node {};
struct Motor : Node {};

void describe(const Sensor &) { std::cout << "sensor\n"; }
void describe(const Motor &) { std::cout << "motor\n"; }
void describe(const Node &) { std::cout << "node（種類が消えた）\n"; }

const Node * const nodes[] = {&sensor, &motor};
for (const Node * const node : nodes) {
  describe(*node);
}
```

実行するとこうです（13.8 の `try.cpp` の前半がこれです）。

```
node（種類が消えた）
node（種類が消えた）
```

**`*node` の静的型は `const Node &` なので、`describe(const Node &)` しか選ばれません。**
中身が `Sensor` でも `Motor` でも関係ありません。
C++ のオーバーロード解決は**コンパイル時に、式の静的型だけで**決まるからです。
実行時の型を見る仕組みは、C++ には**仮想関数しかありません**。

だから 2 段構えになります。

| | 何をするか | 何で決まるか |
| --- | --- | --- |
| 1 回目 `node->accept(v)` | 実行時の型を静的型に**戻す** | 仮想関数（実行時） |
| 2 回目 `v.visit(*this)` | 種類ごとの処理を選ぶ | オーバーロード（コンパイル時） |

`SensorCheck::accept` の中では `*this` の静的型が `SensorCheck` です。
**そこまで来て初めて** `visit(const SensorCheck &)` が選べます。
これが**二重ディスパッチ**です。

### 3 つの `accept` は字面が同じでも、まとめられない

```cpp
void SensorCheck::accept(DiagVisitor & visitor) const { visitor.visit(*this); }
void MotorCheck::accept(DiagVisitor & visitor) const  { visitor.visit(*this); }
void CheckGroup::accept(DiagVisitor & visitor) const  { visitor.visit(*this); }
```

3 つとも 1 文字も違いません。「基底クラスに 1 個書けばいいのでは」と必ず思います。書くとこうなります。

```cpp
struct DiagNode
{
  virtual ~DiagNode() = default;
  void accept(DiagVisitor & visitor) const { visitor.visit(*this); }   // 基底に 1 個
};
```

```
error: no matching member function for call to 'visit'
note: candidate function not viable: no known conversion from 'const DiagNode' to 'const SensorCheck' for 1st argument
note: candidate function not viable: no known conversion from 'const DiagNode' to 'const MotorCheck' for 1st argument
```

**基底の中では `*this` は `DiagNode` だから**です。
このコピペは減らせません。減らそうとした瞬間に二重ディスパッチが壊れます。
Visitor パターンの**構造上のコスト**として受け入れる部分です。

## 13.3 誰が所有するのか

Visitor では所有権が 3 か所に出てきます。**それぞれ答えが違います。**

| 対象 | どう持つ | 理由 |
| --- | --- | --- |
| グループの子 | `std::vector<std::unique_ptr<DiagNode>>` | 木がノードを所有する。第11章 Composite と同じ |
| `accept` の訪問者 | `DiagVisitor &` | 訪問者の寿命は呼び出し側が持っている。所有権は動かない |
| `visit` の要素 | `const Derived &` | 訪問中だけ見る。所有権は木のまま |

**`accept(std::unique_ptr<DiagVisitor>)` と書きたくなったら間違いです。**
訪問者は普通スタックに置いて、その参照を渡すだけで済みます。

```cpp
FailureCountVisitor counter;      // スタック。確保ゼロ
root->accept(counter);
std::cout << counter.failure_count() << "\n";
```

**訪問結果は訪問者のメンバに溜まります。** ここも Java と同じですが、
C++ では「`accept` が終わるまで `counter` が生きていること」を自分で保証します。
上の書き方ならスコープが保証してくれます。

## 13.4 Visitor の代償 — 増やしやすい方向が 90 度ずれている

Visitor は**タダで柔軟になる道具ではありません**。何が安くなり、何が高くなるかがはっきりしています。

| やりたいこと | Visitor だと |
| --- | --- |
| **操作**を増やす（集計を足す、JSON 出力を足す） | **新しい訪問者クラスを 1 つ足すだけ。既存コードは無変更** |
| **要素の種類**を増やす（`EncoderCheck` を足す） | **`DiagVisitor` に `visit` を足す → 全訪問者を直す** |

普通の仮想関数（要素側に `report()` を持たせる方式）はちょうど逆です。
種類を増やすのは楽、操作を増やすと全要素を直すことになります。

この「片方を増やすともう片方が全滅する」構図を **expression problem** と言います。
どちらのパターンを選んでも消えません。**選べるのはどちらを安くするかだけ**です。

> **判断基準**: 要素の種類がほぼ固定で、操作がこれからも増えるなら Visitor。
> 種類が増え続けるなら Visitor は入れないでください。
> [0. 使う前に](00_使う前に.md) の「実装が 1 つしかないのに抽象化する」と同じ話で、
> **訪問者が 1 つしか無いなら Visitor は要りません。** 要素にメンバ関数を足せば済みます。

部活のコードだと、こうなります。

- 「セルフチェック結果」「通信フレームの種別」— **種類は固定**。Visitor か variant が効く
- 「センサの種類」— **これから増える**。Visitor にすると増設のたびに全訪問者を直すことになる

## 13.5 `dynamic_cast` で分岐すればいいのでは

`accept` を書かずに済ませる方法として、必ずこれを思いつきます。

```cpp
void report(const DiagNode & node)
{
  if (const SensorCheck * const s = dynamic_cast<const SensorCheck *>(&node)) { /* ... */ }
  else if (const MotorCheck * const m = dynamic_cast<const MotorCheck *>(&node)) { /* ... */ }
  // CheckGroup を書き忘れた
}
```

動きます。動きますが、3 つ問題があります。

1. **書き忘れてもコンパイルが通る。** 上のコードは `CheckGroup` を黙って無視します。
   種類を増やしたとき、**どこを直せばいいか誰も教えてくれません**
2. **RTTI が要る。** マイコンでは `-fno-rtti` が普通です。

   ```
   error: use of dynamic_cast requires -frtti
   ```

3. **速くない。** `dynamic_cast` は継承関係を実行時に探索します。
   仮想関数 1 回の呼び出しとはコストが違います

**1 番が本質です。** Visitor が `dynamic_cast` の連鎖に勝っているのは、
「種類を増やしたら `DiagVisitor` に純粋仮想 `visit` が増えて、
**実装していない訪問者が全部コンパイルエラーになる**」という一点です。
つまり Visitor は「全滅する」のではなく「**全滅を漏れなく教えてくれる**」道具です。

## 13.6 標準ライブラリ／言語機能に同じものが無いか — `std::variant` と `std::visit`

**あります。C++17 の `std::variant` + `std::visit` が、まさに Visitor です。**
しかも継承も仮想関数も `accept` も要りません。

```cpp
struct SensorSample { std::string name; int value_mv; int limit_mv; };
struct MotorSample  { std::string name; unsigned int fault_bits; };

using DiagValue = std::variant<SensorSample, MotorSample>;   // どれか 1 つが入る
```

`DiagValue` は「3 つのうちちょうど 1 つが入っている箱」です。
共通の基底クラスは**ありません**。3 つの型はお互いに何の関係もなくて構いません。

分岐は `std::visit` でします。

```cpp
std::visit(
  overloaded{
    [](const SensorSample & s) { std::cout << "sensor " << s.value_mv << "mV\n"; },
    [](const MotorSample & m) { std::cout << "motor fault=" << m.fault_bits << "\n"; }},
  value);
```

### `overloaded` イディオムは自分で書く

C++17 の標準ライブラリには**入っていません**。5 行なので自分で書きます。

```cpp
template <class ... Ts>
struct overloaded : Ts ...
{
  using Ts::operator() ...;
};

template <class ... Ts>
overloaded(Ts ...) -> overloaded<Ts ...>;      // 推論ガイド。C++17 では必須
```

ラムダは「`operator()` を 1 個持つ名前の無いクラス」です。
それを**全部継承して**、`using` で全部の `operator()` を可視にすると、
「引数の型でオーバーロード解決される 1 個の関数オブジェクト」ができます。
これが `std::visit` の求めるものです。

下 2 行の**推論ガイドを忘れるとこうなります**。

```
error: no viable constructor or deduction guide for deduction of template arguments of 'overloaded'
note: candidate function template not viable: requires 1 argument, but 2 were provided
```

C++20 では集成体の CTAD が入ったので推論ガイドは要りません。
**C++17 では書いてください。** 課題でも書かせます。

### 網羅性がコンパイル時に保証される

これが `dynamic_cast` の連鎖に対する決定的な差です。
`EncoderV` という種類を variant に足して、ラムダを足し忘れると、

```
error: static assertion failed due to requirement
  'is_invocable_v<overloaded<...>, EncoderV &>':
  `std::visit` requires the visitor to be exhaustive.
```

**「訪問者が網羅的でない」と名指しで落ちます。**
種類を増やしたら、直すべき `std::visit` の呼び出し箇所を**コンパイラが全部挙げてくれます**。
GoF 版で純粋仮想 `visit` を足したときに全訪問者が落ちるのと、まったく同じ効果です。
違うのは、**そのために継承階層を 1 つも書いていない**ことです。

### 木をどう表すか

variant で木を作るときは注意が要ります。
`GroupSample` が `std::vector<DiagValue>` を持つと、
**`DiagValue` の定義の中に `DiagValue` が出てくる**（再帰型）ことになり、
`std::variant` に不完全型を渡すことになります。これは未定義動作です。

課題では**添字で持ちます**。

```cpp
struct GroupSample
{
  std::string name;
  std::vector<std::size_t> children;   // DiagArena の中の位置
};

class DiagArena          // ノードを平らに並べた置き場
{
public:
  std::size_t add(DiagValue value);
  const DiagValue & at(std::size_t index) const;
};
```

木の形は添字で表します。ノード 1 個ごとの `new` が消えるので、
**マイコンでは固定長配列 1 本に置き換えられます**（13.9）。

## 13.7 どちらを選ぶか

| 状況 | 選ぶもの | 理由 |
| --- | --- | --- |
| 要素の種類がコンパイル時に固定 | **`std::variant`** | 継承ゼロ。網羅性がコンパイル時に出る |
| 別のライブラリ利用者が種類を追加する | **GoF 版** | variant は種類の一覧を 1 か所に書き切る必要がある |
| プラグインなど実行時に種類が決まる | **GoF 版** | variant では表現できない |
| ヒープを使いたくない | **`std::variant`** | 中身を直接持つ。ノードごとの確保が無い |
| 要素の大きさがバラバラ | **GoF 版** | variant は**最大メンバの大きさ**を全員が持つ |
| 操作（訪問者）の数が増え続ける | どちらでもよい | どちらもこの方向には強い |
| 要素の種類が増え続ける | **どちらも使わない** | 要素側の仮想関数にする |

最後の 2 行が 13.4 の話です。**まず「どちらの方向に増えるか」を決めてから**選んでください。

`std::variant` の弱点は 5 行目です。

```
sizeof(NodeV) = 8      // SensorV{int} と MotorV{unsigned int} の場合
```

小さい型ばかりなら得ですが、1 つだけ 1KB のメンバがある型を混ぜると、
**全ノードが 1KB になります**。そのときは大きいものだけ `unique_ptr` で逃がします。

## 13.8 手元で試す

1 ファイルで完結します。**出力を予想してから**実行してください。

```cpp
#include <iostream>
#include <string>
#include <variant>
#include <vector>

// ---- 1) オーバーロードは静的型で決まる、を確かめる ----
struct Node { virtual ~Node() = default; };
struct Sensor : Node {};
struct Motor : Node {};

void describe(const Sensor &) { std::cout << "sensor\n"; }
void describe(const Motor &) { std::cout << "motor\n"; }
void describe(const Node &) { std::cout << "node（種類が消えた）\n"; }

// ---- 2) std::variant なら実行時の中身で選べる ----
struct SensorV { int mv; };
struct MotorV { unsigned int fault; };
using NodeV = std::variant<SensorV, MotorV>;

template <class ... Ts>
struct overloaded : Ts ...
{
  using Ts::operator() ...;
};

template <class ... Ts>
overloaded(Ts ...) -> overloaded<Ts ...>;

int main()
{
  Sensor sensor;
  Motor motor;
  const Node * const nodes[] = {&sensor, &motor};

  for (const Node * const node : nodes) {
    describe(*node);
  }

  const std::vector<NodeV> values = {SensorV{11800}, MotorV{3U}};
  for (const NodeV & value : values) {
    std::visit(
      overloaded{
        [](const SensorV & s) { std::cout << "sensor " << s.mv << "mV\n"; },
        [](const MotorV & m) { std::cout << "motor fault=" << m.fault << "\n"; }},
      value);
  }

  std::cout << "sizeof(NodeV) = " << sizeof(NodeV) << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 前半 2 行は何が出るか。<code>describe</code> のオーバーロードは 3 つあるのに</summary>

```
node（種類が消えた）
node（種類が消えた）
sensor 11800mV
motor fault=3
sizeof(NodeV) = 8
```

前半は**両方とも `describe(const Node &)`** です。
`*node` の静的型が `const Node &` なので、そこしか選ばれません。
中身が `Sensor` であることはオーバーロード解決に一切影響しません。
**これを実行時の型で選ばせるために、GoF 版は `accept` という仮想関数を 1 段はさみます。**

後半は種類ごとに選ばれています。継承も `accept` も無いのにです。
`std::visit` が variant の中の判別子を見て、対応する `operator()` に飛ばしているからです。

最後の 8 バイトは「4 バイトのメンバ + 判別子」がアラインされた結果です。
**ヒープは 1 バイトも使っていません。**

さらに試すなら、後半のラムダを 1 つ消してみてください。

```
error: static assertion failed ... `std::visit` requires the visitor to be exhaustive.
```

**書き忘れがコンパイルエラーになる**ことが確認できます。
`dynamic_cast` の連鎖ではこれが起きません。
</details>

## 13.9 マイコンでの結論

**variant 版が本命です。** GoF 版はマイコンで 3 つのコストを払います。

1. 要素 1 個ごとに vtable ポインタ（32bit なら 4 バイト）が乗る
2. 木を `unique_ptr` で作ると**ノードの数だけヒープ確保**が走る
3. 訪問 1 回につき仮想関数呼び出しが 2 回（`accept` と `visit`）

variant 版なら、ノードは**固定長配列に平らに並べられます**。

```cpp
// 動的確保ゼロ。vtable ゼロ。
struct SensorSample { const char * name; int value_mv; int limit_mv; };
struct MotorSample  { const char * name; unsigned int fault_bits; };
using DiagValue = std::variant<SensorSample, MotorSample>;

DiagValue g_nodes[16];        // 静的確保。個数の上限は自分で決める
```

`std::string` は使いません。名前は ROM 上の文字列リテラルを `const char *` で指します
（[README のマイコン制約表](README.md#マイコンと-ros-2-で結論が割れます)）。

### `-fno-exceptions` では `std::get` を使わない

`std::variant` を持ち込むとき、**唯一気をつける点がここ**です。
`std::get<T>(v)` は中身が `T` でないとき `std::bad_variant_access` を **throw** します。

```cpp
const SensorSample & s = std::get<SensorSample>(value);   // 中身が違えば throw
```

`-fno-exceptions` のビルドでは throw できないので、
**中身が違った瞬間に `abort()`** します。デバッガも付いていない基板で止まります。

`std::get_if` を使ってください。**ポインタを返すだけで、投げません。**

```cpp
if (const SensorSample * const sensor = std::get_if<SensorSample>(&value)) {
  std::printf("%d\n", sensor->value_mv);
} else {
  std::printf("sensor ではない\n");
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti try2.cpp -o try2
```

`-fno-exceptions -fno-rtti` の両方を付けても通ります。
**`std::visit` 自体も、variant が有効値を持っている限り投げません**
（例外で壊れたときだけ `valueless_by_exception` になり、そこで投げます。
`-fno-exceptions` なら例外が飛ばないので、この状態になりません）。

まとめると、マイコンでは次の 3 つを守れば `std::variant` は安全に使えます。

| 守ること | 理由 |
| --- | --- |
| `std::get` ではなく `std::get_if` | `std::get` は throw する |
| メンバに `std::string` / `std::vector` を入れない | ヒープ確保が走る |
| 木は添字で持つ（ポインタで持たない） | ノードごとの `new` を消す |

## 13.10 ROS 2 での結論（補足）

ROS 2 では制約が緩いので、どちらでも書けます。実際 rclcpp には両方あります。

- `rclcpp::ParameterValue` は型ごとの値を 1 つ持つ**タグ付きユニオン相当**で、
  `get_type()` で分岐します（`std::variant` と同じ発想）
- メッセージの種別ごとの処理は、素直に**コールバックを型ごとに分ける**のが普通で、
  Visitor クラスを立てることは稀です

ROS 2 側で Visitor を書くとしたら、**受信フレームを自作パーサで種別に分解したあと**、
その結果に対して集計・ログ整形・記録を当てる、という場面です。
種別は固定なので、そこでも `std::variant` が第一候補になります。

## 13.11 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: no matching member function for call to 'visit'` | `accept` を基底クラスに 1 個だけ書いた。`*this` が `DiagNode` になっている（13.2） |
| 基底ポインタ経由だと全部同じ処理になる | `accept` が `virtual` でない。オーバーロードは静的型で決まる |
| 訪問のたびに要素がコピーされる | `visit` の引数が `const Derived &` でなく値になっている |
| 派生固有のメンバが読めない・値が壊れる | `visit(DiagNode node)` と基底型の値で受けている（スライシング） |
| 種類を 1 つ足したら訪問者が全部落ちた | **正常**。それが Visitor の効能（13.4） |
| `error: no viable constructor or deduction guide ... 'overloaded'` | 推論ガイドを書いていない（C++17 では必須） |
| ``error: ... `std::visit` requires the visitor to be exhaustive.`` | ラムダが 1 つ足りない。variant の種類を全部書く |
| ラムダの中から自分を再帰呼び出しできない | ラムダは自分の名前を知らない。名前付き関数を作ってその中で `std::visit` する |
| `error: use of dynamic_cast requires -frtti` | `-fno-rtti` のビルドで `dynamic_cast` を使った（13.5） |
| マイコンで `std::get` を使ったら `abort()` した | `-fno-exceptions` で throw できない。`std::get_if` を使う（13.9） |
| variant のサイズが妙に大きい | 一番大きいメンバに全員が合わせられている。大きい型だけ `unique_ptr` に逃がす |

## 13.12 対応する課題

```bash
./drill run dp13
```

`exercises/dp13_visitor/src/diagnostics.cpp` に、起動時セルフチェックの結果ツリーを題材に、

1. **GoF 版の `accept`（3 つ）** — 二重ディスパッチの本体
2. **`FailureCountVisitor`** — NG の数を数える訪問者（集計）
3. **`TextReportVisitor`** — インデント付きレポートを作る訪問者（整形）
4. **`overloaded` イディオム** — 推論ガイドまで自分で書く
5. **`count_failures` / `make_report`** — `std::visit` 版。1〜3 と**同じ結果**を返すこと

を実装します。テストは、基底ポインタ経由で派生ごとの `visit` が選ばれること
（二重ディスパッチが効いていること）、同じ木に 2 種類の訪問者が当たること、
variant 版が GoF 版と 1 文字も違わない文字列を返すこと、
そして variant 版の型が `static_assert(!std::is_polymorphic_v<...>)` を満たすこと
（= 仮想関数を 1 つも持たないこと）を見ます。

## 13.13 この章のまとめ

- **オーバーロードは静的型で決まる。** 実行時の型で選ばせる仕組みは仮想関数しかない
- だから `accept` が要る。**1 回目の仮想呼び出しで実行時の型を静的型に戻す**のが `accept`
- 3 つの `accept` は字面が同じでも**まとめられない**。基底では `*this` が基底型になる
- `visit` の引数は `const Derived &`。値だとコピー、基底型だとスライシング
- Visitor は**操作を増やすのが安く、要素の種類を増やすのが高い**（expression problem）。
  増える方向を先に決めてから選ぶ
- `dynamic_cast` の連鎖は**書き忘れても通る**。RTTI も要る。Visitor に劣る
- **C++17 には `std::variant` + `std::visit` がある。** 継承も仮想関数も `accept` も不要で、
  網羅性はコンパイル時に保証される（`requires the visitor to be exhaustive`）
- `overloaded` イディオムは標準に無い。**推論ガイドまで自分で書く**（C++17）
- 種類が固定なら variant、実行時に拡張したいなら GoF 版
- **マイコンでは variant が本命。** ヒープも vtable も使わない。
  ただし `-fno-exceptions` では `std::get` ではなく **`std::get_if`**

---

前: [12. Decorator](12_Decorator.md) ／ 次: 14. Chain of Responsibility（準備中）
