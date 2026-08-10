# 10. Strategy

> **結城本 第10章 対応。** `Strategy` インタフェースと `WinningStrategy` / `ProbStrategy`、
> そして `Player` を手元に開いてください。
>
> **この章のねらい**: Java 版の Strategy は 1 通りしか書けません。`interface` を作って `implements` する、それだけです。
> **C++ には少なくとも 3 通りあります。** 仮想関数・`std::function`・テンプレート。
> どれも「アルゴリズムを差し替える」という目的は同じで、
> **差し替えられるタイミング（実行時かコンパイル時か）とコストが違うだけ**です。
> この章では 3 つとも実装して、同じ入力に同じ出力が出ることを確かめたうえで、
> **マイコンではどれを選ぶか**を決めます。

## 10.0 その前に — 実装が 1 つなら入れない

[0. 使う前に](00_使う前に.md) の **0.1「Strategy 病」** がこの章の直前に置いてあるのは偶然ではありません。
輪読で Strategy を読んだ翌日に、実装が 1 つしかないところへ `IMotorDriver` が生えます。

**この章を読んでも、その判断は変わりません。**

> 今、アルゴリズムの実装は 2 つ以上あるか。次の 1 か月で確実に 2 つ目が来るか。
> 答えが No なら、Strategy は入れない。関数を 1 つ書いて終わりにする。

この章がやるのは「2 つ目が本当にある」と分かったあとの話です。
そして C++ では、そこから先に**さらに 3 つの選択肢**があります。
「Strategy を使う」と決めた時点で設計は終わっていません。**そこからが本番です。**

## 10.1 Java 版をそのまま C++ にすると

結城本の `Strategy` はこうです。

```java
public interface Strategy {
    public abstract Hand nextHand();
    public abstract void study(boolean win);
}
```

C++ に素直に移すとこうなります。

```cpp
class Strategy
{
public:
  virtual ~Strategy() = default;
  virtual Hand next_hand() const = 0;
  virtual void study(bool win) = 0;
};
```

Java 版から変えた点が 3 つあります。

### 変更点1: `virtual ~Strategy() = default;` を足した

第 1 章と同じです。`std::unique_ptr<Strategy>` で持って解放したときに
派生クラスのデストラクタが呼ばれなければ未定義動作です。**純粋仮想を書いたら仮想デストラクタ**。

### 変更点2: `next_hand()` に `const` を付けた

`next_hand()` は手を選ぶだけで状態を変えないので `const` メンバ関数にします。
`study()` は勝敗を記録するので付けません。**Java にこの区別はありません。**

ここは単なる作法ではなく、**あとで効きます**。`const` な Strategy は
「状態を持たない（＝どこからでも共有してよい）」の目印になり、
10.6 で出てくる **`static const` なインスタンスを共有する**書き方につながります。

### 変更点3: `Player` が `Strategy` をどう持つかを決めないといけない

Java 版の `Player` はこうです。

```java
private Strategy strategy;
public Player(String name, Strategy strategy) {
    this.strategy = strategy;
}
```

Java では `Strategy strategy;` と書けば「参照を持つ」以外の意味がありません。
C++ では**書き方が 4 通りあり、意味が全部違います**。

```cpp
Strategy strategy_;                       // ① 値。コンパイルできない（抽象クラス）
Strategy * strategy_;                     // ② 生ポインタ。所有していない
std::unique_ptr<Strategy> strategy_;      // ③ 所有する
Strategy & strategy_;                     // ④ 参照。所有しない。差し替えられない
```

**①は書けません。** `Strategy` は純粋仮想関数を持つので実体を作れないからです。
そして仮に作れたとしても、`ProbStrategy` を代入した瞬間に
派生部分が切り落とされます（**スライシング**）。

Java の `Strategy strategy;` に一番近いのは**②か③**です。どちらを選ぶかが 10.2 の話です。

## 10.2 誰が Strategy を所有するのか

判断は 1 行で決まります。

> **Strategy の実体を、Context より長生きさせられるか。**

| 持ち方 | 所有 | 実行時の差し替え | ヒープ | 使う場面 |
| --- | --- | --- | --- | --- |
| `std::unique_ptr<Strategy>` | する | できる | **要る** | 実体の寿命を管理したくない。PC 側 |
| `Strategy *`（生ポインタ） | しない | できる | 要らない | 実体を `static` や呼び出し側が持つ。**マイコン** |
| `Strategy &` | しない | **できない** | 要らない | 生涯 1 つで確定しているとき |
| `std::shared_ptr<Strategy>` | 共有する | できる | 要る | 複数の Context が同じ Strategy を使い、寿命が読めない |

**`Strategy &` を選ぶと差し替えができなくなります。** 参照は再束縛できないからです。

```cpp
class Player
{
public:
  explicit Player(Strategy & strategy) : strategy_(strategy) {}
  void set_strategy(Strategy & strategy)
  {
    strategy_ = strategy;   // 参照先が変わるのではない。中身の代入になる（そして壊れる）
  }
private:
  Strategy & strategy_;
};
```

`strategy_ = strategy;` は「参照を差し替える」ではなく
**「参照先のオブジェクトに代入する」**です。抽象クラスなら代入演算子が使えず
コンパイルエラー、具象クラスなら**スライシングして静かに壊れます**。
Strategy パターンの本質は差し替えなので、**参照メンバは基本的に不適です。**

この課題では**生ポインタ**（`const VelocityFilter * filter_;`）で持ちます。
所有しない、差し替えられる、ヒープが要らない。**マイコンで使うのはこれです。**

その代わり、**Strategy の実体が Context より先に死ぬと即死します**。

```cpp
VirtualCommander make_commander()
{
  ClampFilter filter{1.0};        // ローカル
  return VirtualCommander{filter};// filter はここで死ぬ
}                                 // 返ってきた Commander は死んだ Strategy を指している

auto commander = make_commander();
commander.update(2.0);            // 未定義動作
```

第 1 章のイテレータとまったく同じ形の事故です。
**Java では GC が `filter` を生かすので落ちません。C++ では落ちます。**
生ポインタで持つなら、ヘッダに「実体は Context より長生きさせること」と書くのが仕事です。

### 状態を持つ Strategy はさらに危ない

結城本の `WinningStrategy` は `won` と `prevHand` を持ちます。**状態を持つ Strategy** です。
状態があると、こうなります。

- 2 つの Context で同じ Strategy インスタンスを共有すると、**学習結果が混ざる**
- Strategy を差し替えて戻すと、**前の状態が残っている**（それが望みなら正しい）
- `static` な実体にできない（マルチスレッドで壊れる）

**設計としては、状態は Context 側に置けないかを先に考えてください。**
この課題ではそうしています。フィルタの「前回値」は Strategy ではなく Commander が持ちます。

```cpp
// Strategy は状態を持たない。だから const で、共有してよく、static にできる
virtual double apply(double previous, double raw) const = 0;
```

状態を Context に追い出すと、**Strategy が値のように扱える**ようになります。
これが 10.6 で効きます。

## 10.3 手段その2 — `std::function`

C++ には Java に無い選択肢があります。**継承しない Strategy** です。

```cpp
class FunctionCommander
{
public:
  using FilterFn = std::function<double(double previous, double raw)>;
  explicit FunctionCommander(FilterFn filter) : filter_(std::move(filter)) {}
  void set_filter(FilterFn filter) { filter_ = std::move(filter); }
  double update(double raw) { previous_ = filter_(previous_, raw); return previous_; }
private:
  FilterFn filter_;
  double previous_ = 0.0;
};
```

呼ぶ側はクラスを 1 つも書きません。

```cpp
FunctionCommander commander{
  [](double, double raw) { return std::clamp(raw, -1.0, 1.0); }};
```

Java 版で `class ClampStrategy implements Strategy { ... }` と 5 行書いていたものが、
**ラムダ 1 行**になります。`Strategy` インタフェースそのものが要りません。

そのうえ、`std::function` は**関数ポインタでも、`operator()` を持つクラスでも、
メンバ関数バインドでも受け取れます**。継承階層に縛られない、というのが最大の利点です。

### だが、ヒープ確保が走りうる

`std::function` は中身の型を消して（型消去）保持します。何が入るか分からないので、
**大きすぎるものはヒープに置きます**。手元で実測した結果です（Apple clang / arm64）。

```
sizeof(std::function<double(double)>) = 32
small ラムダ  : 確保 0 回      ← double 1 個キャプチャ
大きいラムダ  : 確保 1 回      ← double 5 個キャプチャ
```

小さいラムダでは確保が 0 回でした。これは **SBO（small buffer optimization）** といって、
`std::function` が内部に持つ小さなバッファに収まったからです。

**しかしこれは規格上の保証ではありません。**
標準が保証しているのは「関数ポインタと `reference_wrapper` を格納するとき例外を投げない」
程度で、**ラムダが確保されないことは保証されていません**。
バッファのサイズも実装依存です（上の 32 バイトは Apple clang の値で、
別の標準ライブラリでは違います）。

つまり `std::function` は、

- **キャプチャを 1 つ増やしただけで、ある日突然 `malloc` が走り始める**
- 走ったかどうかはコードを読んでも分からない
- ライブラリ実装を変えると閾値が変わる

という性質を持ちます。PC では誰も困りません。
**マイコンでは、制御ループの中で `malloc` が走った瞬間に終わりです。**
`-fno-exceptions` なら確保失敗時の挙動も怪しくなります。

## 10.4 手段その3 — テンプレート（ポリシー）

差し替えが**コンパイル時**で足りるなら、これが最良です。

```cpp
template <typename FilterPolicy>
class StaticCommander
{
public:
  explicit StaticCommander(FilterPolicy policy) : policy_(std::move(policy)) {}
  double update(double raw) { previous_ = policy_.apply(previous_, raw); return previous_; }
private:
  FilterPolicy policy_;
  double previous_ = 0.0;
};
```

`FilterPolicy` に要求されるのは **「`apply(double, double)` が呼べること」だけ**です。
基底クラスも `virtual` も `override` もありません。継承関係が要らないので、
**あとから他人が書いた無関係な型を Strategy として渡せます**。

Java の generics ではこれができません（`<T extends Strategy>` と書く必要がある）。
**C++ のテンプレートはダックタイピング**です。ここが Java との大きな差です。

代わりに、**実行時には差し替えられません。**
`StaticCommander<ClampPolicy>` と `StaticCommander<SlewRatePolicy>` は**別の型**です。
同じ変数に入れることはできません。

### コストがゼロというのは本当か

`-O2` で実際に出たコードです（arm64）。仮想関数版:

```
__Z11run_virtualRK6Filterd:
	ldr	x8, [x0]        ; vptr を読む
	ldr	x1, [x8, #16]   ; vtable から apply のアドレスを読む
	br	x1              ; 間接ジャンプ
```

ポリシー版:

```
__Z10run_policyRK11ClampPolicyd:
	ldr	d1, [x0]        ; メンバ m_ を読む
	fcmp	d1, d0
	fcsel	d0, d1, d0, mi  ; 比較して選ぶだけ。呼び出しが消えている
	ret
```

**ポリシー版では関数呼び出しそのものが消えて、比較 1 個になっています。**
仮想関数版はメモリを 2 回読んでから間接ジャンプします。
間接ジャンプは分岐予測が外れやすく、ループの中では効きます。

大きさも違います。

```
sizeof(Clamp)        = 16      ← vptr 8 + double 8
sizeof(ClampPolicy)  = 8       ← double 8
```

vtable ポインタのぶん、**インスタンス 1 個につき 8 バイト**増えます。
加えて vtable 自体が ROM を食い、RTTI 情報も付きます（`-fno-rtti` で削れます）。
センサを 100 個並べるなら 800 バイトです。マイコンの RAM が 20 KB なら無視できません。

## 10.5 4 つ目 — 関数ポインタ

忘れられがちですが、**関数ポインタも立派な Strategy** です。

```cpp
using FilterFn = double (*)(double previous, double raw);

class RawCommander
{
public:
  explicit RawCommander(FilterFn filter) : filter_(filter) {}
  void set_filter(FilterFn filter) { filter_ = filter; }   // 実行時に差し替えられる
  double update(double raw) { previous_ = filter_(previous_, raw); return previous_; }
private:
  FilterFn filter_;
  double previous_ = 0.0;
};
```

- 実行時に差し替えられる（`std::function` と同じ）
- ヒープを一切使わない（`std::function` と違う）
- サイズはポインタ 1 個（8 バイト。`std::function` は 32 バイト）
- vtable も RTTI も要らない

**そして、キャプチャの無いラムダは関数ポインタに暗黙変換できます。**

```cpp
double (*fp)(double) = [](double raw) { return raw * 2.0; };
std::printf("%.1f\n", fp(3.0));      // 6.0
```

つまり「ラムダで書きたい」だけなら `std::function` は要りません。
**キャプチャを使わなければ、関数ポインタで受け取れます。**

キャプチャすると変換できません。実際のエラーはこうです。

```
error: no viable conversion from '(lambda at err.cpp:4:26)' to 'double (*)(double)'
```

弱点は 2 つ。**状態を持てない**（キャプチャできない）ことと、
呼び出しが常に間接呼び出しになる（インライン化されない）ことです。
C 言語の API（`qsort` の比較関数、割り込みハンドラの登録）が全部この形なのは、
これが一番小さい Strategy だからです。

## 10.6 4 つの比較表

| 手段 | 差し替え | ヒープ | サイズ | インライン化 | 状態 | 継承 |
| --- | --- | --- | --- | --- | --- | --- |
| 仮想関数 | 実行時 | 持ち方次第 | vptr +8 | されない | 持てる | 要る |
| `std::function` | 実行時 | **走りうる** | 32 バイト | されない | 持てる | 不要 |
| 関数ポインタ | 実行時 | 無し | 8 バイト | されない | **持てない** | 不要 |
| テンプレート | **コンパイル時** | 無し | ゼロ〜 | **される** | 持てる | 不要 |

選び方は 2 段階です。

1. **実行時に本当に切り替える必要があるか。** 無ければ**テンプレート**
2. 必要なら、**状態が要るか**。要らなければ**関数ポインタ**、要るなら**仮想関数**

`std::function` が第一候補になるのは
「実行時に切り替える」「状態が要る」「クラスを書きたくない」「ヒープを気にしない」
が全部そろったときだけです。**PC 側ならこれで良く、マイコンでは 1 つも許されません。**

## 10.7 標準ライブラリ／言語機能に同じものが無いか

**あります。標準ライブラリは Strategy だらけです。**

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });   // 比較が Strategy
std::unique_ptr<T, MyDeleter> p;                                     // 解放方法が Strategy
std::vector<T, MyAllocator<T>> v;                                    // 確保方法が Strategy
std::unordered_map<K, V, MyHash, MyEq> m;                            // ハッシュと等値が Strategy
```

注目すべきは**どの手段で書かれているか**です。

| 標準の機能 | 差し替え対象 | 手段 |
| --- | --- | --- |
| `std::sort` の第 3 引数 | 比較 | **テンプレート**（実引数から推論） |
| `std::unique_ptr` の第 2 型引数 | 解放 | **テンプレート** |
| `std::vector` の `Allocator` | 確保 | **テンプレート** |
| `std::function` | 呼び出し対象 | 型消去（＝これ自体が「実行時 Strategy」の道具） |
| `std::pmr::polymorphic_allocator` | 確保 | **仮想関数**（`memory_resource`） |

**標準ライブラリの既定はテンプレートです。** ゼロコストだからです。
そして、実行時に切り替えたいという要求に対して、C++17 は
`std::pmr`（多相アロケータ）という**仮想関数版を別に用意**しました。
`std::allocator` と `std::pmr::memory_resource` の関係は、
この章の「テンプレート版と仮想関数版」の関係そのものです。

**標準ライブラリも同じ 3 択で悩み、両方を用意した**ということです。

`std::unique_ptr<T, Deleter>` の `Deleter` が第 2 **型パラメータ**であることも見てください。
`sizeof(std::unique_ptr<int>)` はポインタ 1 個ぶんです。
Deleter をテンプレートにしたおかげで、状態の無い Deleter なら 1 バイトも増えません
（**空基底最適化**）。仮想関数で書いていたらこうはなりません。

## 10.8 手元で試す

`std::function` が本当に確保するのかを、`operator new` を差し替えて数えます。
**予想してから実行してください。**

```cpp
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <new>

static std::size_t g_alloc_count = 0;

void * operator new(std::size_t size)
{
  ++g_alloc_count;
  void * p = std::malloc(size);
  if (p == nullptr) {
    throw std::bad_alloc{};
  }
  return p;
}

void operator delete(void * p) noexcept { std::free(p); }
void operator delete(void * p, std::size_t) noexcept { std::free(p); }

class Filter
{
public:
  virtual ~Filter() = default;
  virtual double apply(double raw) const = 0;
};

class Clamp : public Filter
{
public:
  explicit Clamp(double m) : m_(m) {}
  double apply(double raw) const override { return raw > m_ ? m_ : raw; }
private:
  double m_;
};

class ClampPolicy
{
public:
  explicit ClampPolicy(double m) : m_(m) {}
  double apply(double raw) const { return raw > m_ ? m_ : raw; }
private:
  double m_;
};

int main()
{
  std::printf("sizeof(Clamp)        = %zu\n", sizeof(Clamp));
  std::printf("sizeof(ClampPolicy)  = %zu\n", sizeof(ClampPolicy));
  std::printf("sizeof(std::function<double(double)>) = %zu\n",
              sizeof(std::function<double(double)>));

  double a = 1.0, b = 2.0, c = 3.0, d = 4.0, e = 5.0;

  g_alloc_count = 0;
  std::function<double(double)> small = [a](double raw) { return raw * a; };
  std::printf("small ラムダ  : 確保 %zu 回\n", g_alloc_count);

  g_alloc_count = 0;
  std::function<double(double)> big =
    [a, b, c, d, e](double raw) { return raw * (a + b + c + d + e); };
  std::printf("大きいラムダ  : 確保 %zu 回\n", g_alloc_count);

  double (*fp)(double) = [](double raw) { return raw * 2.0; };
  std::printf("関数ポインタ経由 = %.1f\n", fp(3.0));
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 2 つのラムダで確保は何回ずつ走るか。そして <code>Clamp</code> と <code>ClampPolicy</code> のサイズ差は</summary>

手元（Apple clang / arm64）ではこうなりました。

```
sizeof(Clamp)        = 16
sizeof(ClampPolicy)  = 8
sizeof(std::function<double(double)>) = 32
small ラムダ  : 確保 0 回
大きいラムダ  : 確保 1 回
関数ポインタ経由 = 6.0
```

- `double` 1 個キャプチャのラムダは**確保されませんでした**（SBO に収まった）
- `double` 5 個（40 バイト）キャプチャすると、`std::function` の 32 バイトに収まらず**確保が 1 回**走りました
- サイズ差 8 バイトが vtable ポインタです

**キャプチャを 1 つ足しただけで挙動が変わる**のが分かります。
そして、この境目は規格に書かれていません。**別のコンパイラ・別の標準ライブラリでは違う値になります。**
「手元では確保されなかった」は、マイコンでは根拠になりません。
</details>

## 10.9 マイコンでの結論

**第一候補はテンプレート（ポリシー）版です。**

```cpp
template <typename FilterPolicy>
class VelocityCommander
{
public:
  explicit VelocityCommander(FilterPolicy policy) : policy_(policy) {}
  double update(double raw) { previous_ = policy_.apply(previous_, raw); return previous_; }
private:
  FilterPolicy policy_;
  double previous_ = 0.0;
};

// 使う側。型がここで確定する
VelocityCommander<ClampPolicy> commander{ClampPolicy{1.0}};
```

vtable なし、ヒープなし、インライン展開される。10.4 で見たとおり、
`apply` の呼び出しそのものが消えます。**部活のマイコン側は原則これで書いてください。**

**`std::function` は使いません。** 理由は 10.3 のとおり、
確保が走るかどうかが規格で保証されないからです。
「小さいから大丈夫」は、キャプチャを 1 つ足した後輩によって破られます。

### 実行時に本当に切り替えたいとき

「起動時にディップスイッチを読んで制御則を決める」「デバッグコマンドでフィルタを切り替える」
のように、**実行時に決まる**場合があります。このときだけ仮想関数版を使います。
**ヒープは使いません。実体を `static` に置き、ポインタを差し替えます。**

```cpp
// 実体はプログラムの寿命と同じだけ生きる。ヒープを使わない
static const ClampFilter    kClamp{1.0};
static const SlewRateFilter kSlew{0.5};

class VelocityCommander
{
public:
  explicit VelocityCommander(const VelocityFilter & filter) : filter_(&filter) {}
  void set_filter(const VelocityFilter & filter) { filter_ = &filter; }  // 差し替え
  double update(double raw) { previous_ = filter_->apply(previous_, raw); return previous_; }
private:
  const VelocityFilter * filter_;   // 所有しない
  double previous_ = 0.0;
};

VelocityCommander commander{kClamp};
if (dip_switch_is_on()) {
  commander.set_filter(kSlew);      // make_unique も new も出てこない
}
```

**`std::unique_ptr<VelocityFilter>` は出てきません。**
Strategy の種類はコンパイル時に全部分かっているので、実体を先に静的に置いておけば足ります。
動くのは**ポインタ 1 個**だけです。

ここで 10.2 の「状態は Context 側に置く」が効いています。
`apply()` が `const` で状態を持たないから、`static const` にできて、
複数の Commander が同じ実体を共有できます。

### さらに削るなら

Strategy が状態を持たず、種類も少ないなら、**関数ポインタ**で足ります（10.5）。
`std::function` の 32 バイトが 8 バイトになり、vtable も ROM から消えます。

```cpp
using FilterFn = double (*)(double previous, double raw);
static double clamp_filter(double, double raw) { return raw > 1.0 ? 1.0 : raw; }
```

### やってはいけない書き方

```cpp
// ダメ: ループの中で Strategy を作り直している
void control_loop()
{
  while (true) {
    auto filter = std::make_unique<ClampFilter>(1.0);   // 毎周ヒープ確保
    output = commander.update(filter.get(), raw);
  }
}
```

制御周期 1 kHz なら 1 秒に 1000 回の確保と解放です。断片化していずれ失敗します。
**Strategy は起動時に作って、以後は差し替えるだけ**です。

## 10.10 ROS 2 での結論（補足）

ROS 2（Linux 上）では 3 つとも使って構いません。実際 rclcpp は
**`std::function` を全面的に使っています**。

```cpp
subscription_ = this->create_subscription<sensor_msgs::msg::Imu>(
  "imu", 10,
  [this](sensor_msgs::msg::Imu::SharedPtr msg) { this->on_imu(msg); });
```

このコールバックが Strategy です。`Subscription` は「メッセージが来たら何をするか」を
知らず、外から渡されたものを呼ぶだけ。継承も `override` も要りません。
**Java 版なら `MessageListener` インタフェースを実装したクラスを書くところです。**

注意点は 2 つ。

- コールバックの中で `this` をキャプチャしているので、**ノードより長生きさせない**こと。
  これは 10.2 の寿命の話がそのまま出てきています
- リアルタイム経路（`rclcpp::executors::StaticSingleThreadedExecutor` などで
  周期を守りたい経路）では、マイコンと同じ理由で `std::function` の確保が問題になります。
  そこだけはテンプレートか関数ポインタに寄せてください

パラメータで制御則を切り替える（`declare_parameter("filter_type", "slew")`）なら、
**実行時に決まる**ので仮想関数版か `std::function` 版です。
このときは `std::unique_ptr<VelocityFilter>` で所有して構いません。ヒープが自由だからです。

## 10.11 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: field type 'Strategy' is an abstract class` | Strategy を値でメンバに持とうとしている。ポインタか参照か `unique_ptr` にする |
| Strategy を代入したら派生の処理が呼ばれなくなった | 値で受けてスライシングしている。基底の部分しか残っていない |
| `set_strategy()` を書いたのにコンパイルエラー | メンバが `Strategy &`。参照は再束縛できない。ポインタにする |
| Strategy を差し替えたのに挙動が変わらない | Context が Strategy を**コピー**して持っている。ポインタで指す |
| Context を返す関数から返したら落ちた | Strategy の実体がローカル変数だった。10.2 の寿命の問題 |
| `error: no viable conversion from '(lambda ...)' to 'double (*)(double)'` | キャプチャありのラムダは関数ポインタにならない。`std::function` かテンプレートへ |
| マイコンで時々ハングする / だんだん遅くなる | `std::function` のヒープ確保。テンプレートか関数ポインタへ |
| `StaticCommander<ClampPolicy>` と `<SlewRatePolicy>` を同じ変数に入れられない | 別の型。**それがテンプレート版の制約**。実行時に切り替えたいなら仮想関数版へ |
| 解放時にデストラクタが呼ばれない | `Strategy` に仮想デストラクタが無い |
| 2 つの Context で学習結果が混ざる | Strategy が状態を持っていて共有されている。状態は Context 側へ |

## 10.12 対応する課題

```bash
./drill run dp10
```

題材は**速度指令のフィルタ**です。上位から降ってくる生の指令を、
前回出した値と突き合わせて丸めてからモータに渡します。
丸めかたが Strategy で、2 種類あります。

- `clamp` — 絶対値で頭打ち（`raw` を `[-max_abs, +max_abs]` に収める）
- `slew rate` — 前回値からの変化量を制限（`raw` を `[previous - d, previous + d]` に収める）

`exercises/dp10_strategy/src/velocity_filter.cpp` に、**同じアルゴリズムを 3 通り**実装します。

1. **仮想関数版** — `ClampFilter::apply` / `SlewRateFilter::apply` と、
   Strategy を**所有せずポインタで指す** `VirtualCommander`
2. **`std::function` 版** — `FunctionCommander` と、ラムダを返す `make_clamp_fn`
3. **テンプレート版** — `ClampPolicy::apply` / `SlewRatePolicy::apply`（`virtual` を書かないこと）

テストが見るのは次の 4 点です。

- **3 つとも同じ入力に同じ出力を返す**こと（手段が違うだけで、アルゴリズムは同じもの）
- `set_filter()` で**実行時に挙動が変わる**こと
- `VirtualCommander` が Strategy を**コピーせずアドレスで指している**こと（`filter()` のアドレス比較）
- テンプレート版が**仮想関数を持たない**こと
  （`static_assert(!std::is_polymorphic_v<ClampPolicy>)` と `sizeof` の比較）
- `FunctionCommander` に**キャプチャありのラムダを直接渡せる**こと

## 10.13 この章のまとめ

- **実装が 1 つなら Strategy は入れない。** この章を読んでもそこは変わらない
- Java の `interface` 1 通りに対し、C++ には **仮想関数 / `std::function` / 関数ポインタ / テンプレート**の 4 通りがある
- 違いは「**差し替えがコンパイル時か実行時か**」と「**ヒープを使うか**」
- Context が Strategy を**どう持つか**が設計の本体。`unique_ptr`（所有）/ 生ポインタ（所有しない）/
  参照（差し替え不可）。**参照メンバは Strategy には向かない**
- **状態は Strategy ではなく Context に置く。** そうすると Strategy を `static const` で共有できる
- `std::function` の SBO は**規格上の保証ではない**。キャプチャが増えれば確保が走る
- キャプチャなしのラムダは**関数ポインタに変換できる**。C の API がこの形なのはこれが最小だから
- 標準ライブラリの Strategy（`sort` の比較、`unique_ptr` の Deleter、Allocator）は**既定がテンプレート**
- **マイコンではテンプレート版が第一候補。** 実行時に切り替えるときだけ、
  `static` な実体へのポインタを差し替える仮想関数版を使う。`std::function` は使わない

---

前: [9. Bridge](09_Bridge.md) ／ 次: 11. Composite（準備中）
