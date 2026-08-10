# 9. Bridge

> **結城本 第9章 対応。** `Display` / `CountDisplay`（機能のクラス階層）と
> `DisplayImpl` / `StringDisplayImpl`（実装のクラス階層）を手元に開いてください。
>
> **この章のねらい**: Bridge の構造は Java 版とほぼ同じです。ですが C++ では、
> **まったく同じ構造が「Pimpl」という別の名前で、まったく別の目的に使われています。**
> 目的が違うと、実装側を仮想にするかどうかが変わります。
> さらに C++ には Java に無い落とし穴が 1 つあります。
> `std::unique_ptr<Impl>` を持つと、**デストラクタをヘッダに書けなくなります。**
> この章はそこが本番です。

## 9.1 Java 版をそのまま C++ にすると

結城本の `Display` はこうです。

```java
public class Display {
    private DisplayImpl impl;
    public Display(DisplayImpl impl) { this.impl = impl; }
    public void open()  { impl.rawOpen();  }
    public void print() { impl.rawPrint(); }
    public void close() { impl.rawClose(); }
    public final void display() { open(); print(); close(); }
}
```

C++ に素直に移すとこうなります。

```cpp
class TelemetrySink
{
public:
  virtual ~TelemetrySink() = default;
  virtual void open() = 0;
  virtual void put_line(const std::string & line) = 0;
  virtual void close() = 0;
};

class TelemetryView
{
public:
  explicit TelemetryView(std::unique_ptr<TelemetrySink> sink)
  : sink_(std::move(sink))
  {
  }

  virtual ~TelemetryView() = default;

  void show(const std::string & text);

protected:
  TelemetrySink & sink() { return *sink_; }

private:
  std::unique_ptr<TelemetrySink> sink_;
};
```

Java 版から変えた点が 3 つあります。

### 変更点1: 実装側にも機能側にも仮想デストラクタを足した

実装側（`TelemetrySink`）は当然です。`unique_ptr<TelemetrySink>` で解放するので、
無ければ派生のデストラクタが呼ばれません。

**機能側（`TelemetryView`）にも要ります。** Bridge の機能側は必ず派生されるからです
（結城本の `CountDisplay`）。

```cpp
std::unique_ptr<TelemetryView> view = std::make_unique<RepeatView>(/* ... */);
// view が破棄されるとき、RepeatView のメンバが解放されない
```

**「機能側は継承しない前提だから仮想は要らない」は Bridge では成り立ちません。**
Bridge は機能側を継承で伸ばすパターンです。

### 変更点2: `impl` フィールドを `std::unique_ptr` にした

Java の `private DisplayImpl impl;` は参照です。GC が回収します。
C++ で同じ意味を持たせるなら、**誰が解放するかを型に書きます**。

```cpp
TelemetrySink * sink_;                    // 誰が delete する？　書いてない
std::unique_ptr<TelemetrySink> sink_;     // TelemetryView が所有する。明白
```

`TelemetrySink & sink_;`（参照メンバ）という手もありますが、
参照メンバを持つとクラスが代入不可になり、寿命の管理が外に出ます。
**Bridge は「実装を丸ごと 1 つ抱える」パターンなので、所有する `unique_ptr` が素直です。**

### 変更点3: `display()` の `final` を、そもそも `virtual` を書かないことで表現した

Java 版は `public final void display()` です。「これは上書きするな」の宣言です。
C++ では `virtual` を書かなければ最初から上書きできません。
`final` と書く必要すらありません。

**Java は「デフォルトが仮想」なので `final` で止める。C++ は「デフォルトが非仮想」なので
何もしなくていい。** 逆に、上書きさせたいところに `virtual` を書き忘れると
**静かにバグります**（第3章 Template Method と同じ話です）。

## 9.2 誰が実装を所有するのか

Bridge では、機能側のオブジェクトが 1 個の実装オブジェクトを抱えます。
所有権の形は 3 通り考えられます。

| 形 | 意味 | いつ使うか |
| --- | --- | --- |
| `std::unique_ptr<Impl>` | 機能側が単独で所有する | **既定。この章の課題もこれ** |
| `std::shared_ptr<Impl>` | 実装を複数の機能側で共有する | 1 本の UART を複数の View が使う、など |
| `Impl &`（参照メンバ） | 所有しない。外が生かしておく | 実装がグローバル（ペリフェラル）のとき |

3 番目はマイコンで実際に使います。`UART1` は世界に 1 個で、
プログラム開始から終了まで生きているので、所有する必要がありません。

コンストラクタはこうなります。

```cpp
explicit TelemetryView(std::unique_ptr<TelemetrySink> sink)
: sink_(std::move(sink))
{
}
```

**`std::move` を書き忘れると `unique_ptr` はコピーできないのでコンパイルエラー**になります。
ここは C++ が守ってくれる数少ない場所です。

呼ぶ側はこう書きます。

```cpp
TelemetryView view{std::make_unique<RecordingSink>(log)};
```

「実装を実行時に選ぶ」がこの 1 行に集約されました。これが Bridge の値打ちです。

## 9.3 なぜ継承ではなくメンバなのか

Bridge を使わないとどうなるか、を先に見ておきます。
機能が 2 種（そのまま出す / 繰り返して出す）、実装が 2 種（そのまま記録 / 番号付き）あるとき、
継承だけで組むとこうなります。

```
RecordingView          NumberedView
RepeatRecordingView    RepeatNumberedView
```

**4 クラス**です。機能を 1 つ足すと 2 クラス、実装を 1 つ足すと 2 クラス増えます。
M×N です。

Bridge にすると `TelemetryView` / `RepeatView` / `RecordingSink` / `NumberedSink` の
**M+N クラス**で、組み合わせは実行時に作れます。

```cpp
TelemetryView a{std::make_unique<RecordingSink>(log)};
TelemetryView b{std::make_unique<NumberedSink>(log)};
RepeatView    c{std::make_unique<RecordingSink>(log), 3};
RepeatView    d{std::make_unique<NumberedSink>(log), 3};
```

**継承の縦棒が 2 本になったことが本質で、`unique_ptr` はその 2 本をつなぐ橋です。**

> **9.3 の注意**: [0. 使う前に](00_使う前に.md) の 0.1 がここにも効きます。
> **実装が 1 種類しかないなら Bridge は入れません。** クラスが 1 個増えるだけです。
> Bridge が要るのは「機能も実装も両方伸びる」ことが**今**分かっているときだけです。

## 9.4 Bridge と Pimpl は同じ構造。目的が違う

C++ を書いていると、こういうクラスに必ず出会います。

```cpp
class LinkStats
{
public:
  LinkStats();
  ~LinkStats();
  void add_sample(double latency_ms);
  double mean() const;

private:
  struct Impl;                    // 前方宣言だけ
  std::unique_ptr<Impl> impl_;    // 実装への橋
};
```

**Bridge と構造がまったく同じです。** メンバに実装へのポインタを 1 本持ち、
公開メソッドはそこへ委譲する。これが **Pimpl**（pointer to implementation）です。

違うのは目的です。

| | Bridge | Pimpl |
| --- | --- | --- |
| 何のため | 実装を**差し替える**（2 軸で拡張する） | 実装を**隠す**（コンパイル時間・ABI） |
| 実装側の数 | 2 個以上。だから存在価値がある | **常に 1 個** |
| 実装側は多態か | **はい。純粋仮想の基底が要る** | **いいえ。ただの構造体** |
| 実装側の型名 | ヘッダに出る（利用者が選ぶので） | ヘッダに出ない（`struct Impl;` だけ） |
| 機能側の階層 | 継承で伸ばす | 伸ばさないことが多い |
| コスト | 仮想呼び出し + ヒープ 1 回 | **間接参照 + ヒープ 1 回。仮想は無い** |

**「Bridge を C++ で書け」と言われたら実装側は純粋仮想。「Pimpl にしろ」と言われたら
実装側は仮想にしない。** 同じ形なので混ざりやすいところです。

課題では両方書きます。`TelemetryView` が Bridge、`LinkStats` が Pimpl です。

## 9.5 C++ 固有の危険 — デストラクタをヘッダに書けない

**この章で一番よく踏みます。** さっきの `LinkStats` から、
「デストラクタは何もしないから」と `~LinkStats();` の宣言を消してみます。

```cpp
#include <memory>

class LinkStats
{
public:
  LinkStats();
  void add_sample(double v);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

int main()
{
  LinkStats stats;   // ここでデストラクタが要求される
  stats.add_sample(1.0);
  return 0;
}
```

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -c pimpl_bad.cpp
```

```
/.../c++/v1/__memory/unique_ptr.h:75:19: error: invalid application of 'sizeof' to an incomplete type 'LinkStats::Impl'
   75 |     static_assert(sizeof(_Tp) >= 0, "cannot delete an incomplete type");
      |                   ^~~~~~~~~~~
/.../c++/v1/__memory/unique_ptr.h:259:71: note: in instantiation of member function 'std::unique_ptr<LinkStats::Impl>::~unique_ptr' requested here
  259 |   _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 ~unique_ptr() { reset(); }
pimpl_bad.cpp:10:10: note: forward declaration of 'LinkStats::Impl'
   10 |   struct Impl;
      |          ^
```

（GCC だと `static assertion failed: can't delete an incomplete type` という文言になります。
どちらも同じ原因です。）

**理由**: コンパイラが暗黙のデストラクタを生成する場所は、`class LinkStats { ... };` の閉じ括弧です。
その時点で `Impl` はまだ前方宣言だけ = **不完全型**です。
`std::unique_ptr` のデストラクタは `delete` を呼ぶために `sizeof(Impl)` を要求します。
不完全型に `sizeof` は使えません。

**解決**: デストラクタを**宣言だけヘッダに書き、定義は `.cpp` に置く**。

```cpp
// link_stats.hpp
class LinkStats
{
public:
  LinkStats();
  ~LinkStats();                 // 宣言だけ
  // ...
};
```

```cpp
// link_stats.cpp
struct LinkStats::Impl        // ここで初めて完全型になる
{
  std::vector<double> samples;
};

LinkStats::~LinkStats() = default;   // ここなら Impl は完全型
```

**`= default` を .cpp 側に書くのがポイントです。** ヘッダで `~LinkStats() = default;` と書くと、
それは「ここで定義する」という意味なので同じエラーになります。

### ムーブも同じ

デストラクタを宣言した瞬間、**ムーブコンストラクタとムーブ代入は暗黙生成されなくなります**
（C++ の特殊メンバ関数のルール）。つまり `LinkStats` はムーブできないクラスになります。
`std::vector<LinkStats>` に入れようとして初めて気付きます。

なので、ムーブも宣言と定義を分けて書き足します。

```cpp
// ヘッダ
LinkStats(LinkStats && other) noexcept;
LinkStats & operator=(LinkStats && other) noexcept;

// .cpp
LinkStats::LinkStats(LinkStats && other) noexcept = default;
LinkStats & LinkStats::operator=(LinkStats && other) noexcept = default;
```

ムーブ代入は「左辺の古い `impl_` を `delete` する」ので、これもヘッダには書けません。

**Pimpl のヘッダに必ず並ぶ 5 行**として覚えてください。

```cpp
LinkStats();
~LinkStats();
LinkStats(LinkStats &&) noexcept;
LinkStats & operator=(LinkStats &&) noexcept;
LinkStats(const LinkStats &) = delete;          // コピーは自分で書かないと消える
```

コピーは `unique_ptr` のせいでどのみち生成されません。
**コピーが欲しいなら自分で書きます**（`impl_ = std::make_unique<Impl>(*other.impl_);`）。
その場合コピーコンストラクタの定義も .cpp です。

Bridge 側（`TelemetryView`）でこのエラーが出ないのは、
`TelemetrySink` がヘッダで**完全に定義されている**からです。
**Pimpl 特有の問題ではなく「不完全型 + `unique_ptr`」の問題**だと理解してください。

## 9.6 ヘッダ依存が減る = 再コンパイルが減る

Pimpl の目的はこれです。実物で見ます。課題の `link_stats.hpp` の include はこれだけです。

```cpp
#include <cstddef>
#include <memory>
```

一方 `link_stats.cpp` はこうです。

```cpp
#include <algorithm>
#include <vector>
```

**`<vector>` も `<algorithm>` もヘッダ側に漏れていません。**
Pimpl をやめて `std::vector<double> samples_;` をヘッダのメンバに書くと、
`link_stats.hpp` は `<vector>` を include しなければならなくなります。

何が変わるか。

| | Pimpl あり | Pimpl なし |
| --- | --- | --- |
| `Impl` にメンバを 1 個足す | **`link_stats.cpp` だけ再コンパイル** | ヘッダを include している**全 .cpp が再コンパイル** |
| クラスのサイズ | 常にポインタ 1 個分 | メンバを足すたびに変わる |
| ライブラリを .so で配る | 実装を変えても**利用者は再ビルド不要**（ABI が変わらない） | サイズが変わる = ABI が壊れる |

部活のライブラリで効くのは 1 行目です。
`link_stats.hpp` を 30 個の .cpp が include していたとします。
Pimpl なしなら、統計にメンバを 1 個足すたびに 30 ファイルが再コンパイルされます。
Pimpl ありなら 1 ファイルです。

3 行目は、ROS 2 のように**共有ライブラリで配る**ときに効きます。
`sizeof(LinkStats)` が変わると、既にコンパイル済みの利用者コードは
スタック上に確保するサイズを間違えます。Pimpl ならサイズは永久に変わりません。

課題のテストはこれを `static_assert` で見ています。

```cpp
static_assert(
  sizeof(LinkStats) == sizeof(std::unique_ptr<void *>),
  "LinkStats はポインタ 1 個分のはずです。実装をヘッダに書いていませんか");
```

**代償**もはっきりしています。

- ヒープ確保が 1 回入る（生成のたび）
- メンバアクセスに間接参照が 1 段入る。**インライン展開されない**
- コードが 2 か所に散る。小さいクラスでは害の方が大きい

**判断基準**: そのヘッダが多くの .cpp から include されていて、
かつ実装がまだ動くなら Pimpl。**部活のコードだと大半は「要らない」が正解です。**

## 9.7 実装を差し替えたいなら仮想、隠したいだけなら非仮想

9.4 の表を、コードで並べておきます。

```cpp
// Bridge: 実装側は純粋仮想。差し替えられる
class TelemetrySink { public: virtual ~TelemetrySink() = default; /* ... */ };
class TelemetryView
{
private:
  std::unique_ptr<TelemetrySink> sink_;   // 中身が何かは実行時に決まる
};
```

```cpp
// Pimpl: 実装側はただの struct。差し替えない
class LinkStats
{
private:
  struct Impl;                            // 実装は世界に 1 種類
  std::unique_ptr<Impl> impl_;
};
```

**Pimpl の `Impl` を純粋仮想の基底に変えると、そのまま Bridge になります。**
逆に、Bridge の実装が 1 種類しか無いと気付いたら、
純粋仮想をやめてただの `struct Impl` にすると Pimpl になります。仮想呼び出しが 1 回消えます。

「Bridge にしておけば将来差し替えられるから」と純粋仮想にしておくのは、
[0. 使う前に](00_使う前に.md) の 0.1 そのものです。**実装が 1 個なら仮想にしません。**

## 9.8 標準ライブラリ／言語機能に同じものが無いか

**Bridge そのものは標準ライブラリにありません。**「2 つの階層を分ける」は設計の話で、
ライブラリで提供できるものではないからです。

ただし、Bridge の**実装側を保持する手段**として標準に用意されているものはあります。

| 標準の道具 | Bridge の何に当たるか |
| --- | --- |
| `std::unique_ptr<Impl>` | 橋そのもの。この章で使う |
| `std::function<void(const std::string &)>` | 実装が 1 メソッドしか無いなら、クラスを作らずこれで済む |
| テンプレート引数 | 実装をコンパイル時に決める。9.10 のマイコン版 |

`std::function` 版はこう書けます。

```cpp
class TelemetryView
{
public:
  explicit TelemetryView(std::function<void(const std::string &)> put_line)
  : put_line_(std::move(put_line))
  {
  }

private:
  std::function<void(const std::string &)> put_line_;
};
```

**実装側のインタフェースが 1 メソッドなら、これで十分です。** クラス階層が 1 本消えます。
この判断は第 10 章 Strategy で正面から扱います。
`open()` / `put_line()` / `close()` のように**複数のメソッドが状態を共有する**なら、
`std::function` を 3 つ持つより純粋仮想クラス 1 個の方が素直です。

一方 **Pimpl は C++ の定番イディオムとして広く使われています**。標準ライブラリの実装自体や、
Qt（`Q_DECLARE_PRIVATE`）、`std::pmr` 以前の多くのライブラリが使っています。
言語機能としてのサポートは無いので、9.5 の 5 行を毎回手で書きます。

## 9.9 手元で試す

課題を解く前に、この 1 ファイルをコンパイルして**出力を予想してから**実行してください。

```cpp
#include <cstdio>
#include <memory>
#include <string>

// ── 実装のクラス階層 ──
class Sink
{
public:
  virtual ~Sink() = default;
  virtual void put_line(const std::string & line) = 0;
};

class StdoutSink : public Sink
{
public:
  void put_line(const std::string & line) override { std::printf("[out] %s\n", line.c_str()); }
};

class NumberedSink : public Sink
{
public:
  void put_line(const std::string & line) override
  {
    std::printf("[out] %d: %s\n", n_++, line.c_str());
  }

private:
  int n_ = 0;
};

// ── 機能のクラス階層 ──
class View
{
public:
  explicit View(std::unique_ptr<Sink> sink) : sink_(std::move(sink)) {}
  virtual ~View() = default;
  void show(const std::string & text) { sink_->put_line(text); }

protected:
  Sink & sink() { return *sink_; }

private:
  std::unique_ptr<Sink> sink_;
};

class RepeatView : public View
{
public:
  RepeatView(std::unique_ptr<Sink> sink, int times) : View(std::move(sink)), times_(times) {}
  void show_repeat(const std::string & text)
  {
    for (int i = 0; i < times_; ++i) {
      sink().put_line(text);
    }
  }

private:
  int times_;
};

int main()
{
  View a{std::make_unique<StdoutSink>()};
  a.show("v=1");

  View b{std::make_unique<NumberedSink>()};
  b.show("v=1");

  RepeatView c{std::make_unique<StdoutSink>(), 2};
  c.show_repeat("v=1");

  RepeatView d{std::make_unique<NumberedSink>(), 2};
  d.show_repeat("v=1");

  std::printf("sizeof(View)=%zu sizeof(RepeatView)=%zu\n", sizeof(View), sizeof(RepeatView));
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: <code>sizeof(View)</code> は何になるか。<code>RepeatView</code> との差は</summary>

```
[out] v=1
[out] 0: v=1
[out] v=1
[out] v=1
[out] 0: v=1
[out] 1: v=1
sizeof(View)=16 sizeof(RepeatView)=24
```

`View` が 16 バイトなのは、**vptr 8 + `unique_ptr` 8** です。
`virtual ~View()` を書いた時点で vptr が 1 個載っています。
`RepeatView` はそこに `int times_` が足されて、アラインメントで 24 になります。

**`sizeof(View)` に `StdoutSink` の大きさは入っていません。**
実装は別の場所（ヒープ）にあって、機能側はポインタ 1 本しか持たないからです。
これが Bridge の「橋」の実体です。

`std::printf` の `%d` に `n_++` を渡していますが、これは `int` なので問題ありません。
`std::size_t` を `%d` で出すと未定義動作になるので、
`sizeof` は `%zu` にしてあります。
</details>

## 9.10 マイコンでの結論

**Pimpl はそのままでは使えません。** `std::make_unique<Impl>()` がヒープ確保だからです。
ループ中はもちろん、起動時であっても「ヒープを一切使わない」方針のプロジェクトでは書けません。

選択肢は 3 つです。

### (a) 素直にヘッダに書く（第一候補）

マイコンのファームは、**共有ライブラリで配りません。ABI 互換も要りません。**
ビルド対象は多くて数十ファイルで、フルビルドしても数秒です。
**Pimpl の利点が 3 つとも消えています。** 隠す理由がありません。

```cpp
class LinkStats
{
public:
  void add_sample(float latency_ms);
  float max() const;

private:
  float max_ = 0.0F;      // ヘッダに出して構わない
  std::uint16_t count_ = 0;
};
```

**まずこれを検討してください。** 「隠したい」は目的ではなく手段です。

### (b) fast-pimpl（固定サイズのストレージに placement new）

どうしても隠したいなら、ヒープの代わりに**メンバの固定バッファ**に構築します。

```cpp
// link_stats.hpp
#include <cstddef>
#include <new>

class LinkStats
{
public:
  LinkStats();
  ~LinkStats();
  LinkStats(const LinkStats &) = delete;
  LinkStats & operator=(const LinkStats &) = delete;

  void add_sample(float latency_ms);
  float max() const;

private:
  struct Impl;

  // 固定サイズのストレージ。ヒープを使わない。
  static constexpr std::size_t kImplSize = 32;
  static constexpr std::size_t kImplAlign = 4;
  alignas(kImplAlign) unsigned char storage_[kImplSize];

  Impl * impl();
  const Impl * impl() const;
};
```

```cpp
// link_stats.cpp
#include "link_stats.hpp"

struct LinkStats::Impl
{
  float max = 0.0F;
  unsigned count = 0;
};

LinkStats::Impl * LinkStats::impl() { return reinterpret_cast<Impl *>(storage_); }
const LinkStats::Impl * LinkStats::impl() const { return reinterpret_cast<const Impl *>(storage_); }

LinkStats::LinkStats()
{
  // サイズと alignment が合っているかを **コンパイル時に**検査する。
  // Impl を大きくしたら、ここで落ちて気付ける。
  static_assert(sizeof(Impl) <= kImplSize, "kImplSize を増やしてください");
  static_assert(alignof(Impl) <= kImplAlign, "kImplAlign を増やしてください");
  new (storage_) Impl();   // placement new。ヒープは使わない
}

LinkStats::~LinkStats() { impl()->~Impl(); }   // 明示的にデストラクタを呼ぶ

void LinkStats::add_sample(float latency_ms)
{
  if (latency_ms > impl()->max) {
    impl()->max = latency_ms;
  }
  ++impl()->count;
}

float LinkStats::max() const { return impl()->max; }
```

`-fno-exceptions -fno-rtti` でも通ります（実測しました。`sizeof(LinkStats)` は 32 です）。

**代償が 2 つ**あります。

- `kImplSize` を**手で管理**することになる。`Impl` を大きくしたら
  `static_assert` で落ちるので気付けますが、直すのは人間です
- **ヘッダに `sizeof` が固定で出てしまう**ので、ABI を守る効果は無くなります
  （そもそもマイコンでは要らないので問題になりません）

デストラクタを .cpp に置く必要がある点は Pimpl と同じです。`Impl` が不完全型だからです。

### (c) Bridge の 2 軸分離はテンプレートでやる

**Bridge の「機能と実装を分ける」だけなら、仮想関数もヒープも要りません。**
実装をテンプレート引数で受けます。

```cpp
// 実装（コンパイル時に決まる）
class Uart1Sink
{
public:
  void put_line(const char * line) { hal_uart_write(1, line); }   // UART1 に流す
};

class NullSink
{
public:
  void put_line(const char *) {}    // 出力を殺す。テスト用にも使える
};

// 機能（実装をテンプレート引数で受ける）
template <typename SinkT>
class TelemetryView
{
public:
  void show(const char * text) { sink_.put_line(text); }

protected:
  SinkT sink_;
};

template <typename SinkT>
class RepeatView : public TelemetryView<SinkT>
{
public:
  explicit RepeatView(int times) : times_(times) {}
  void show_repeat(const char * text)
  {
    for (int i = 0; i < times_; ++i) {
      this->sink_.put_line(text);   // this-> が要る。依存名だから
    }
  }

private:
  int times_;
};
```

```cpp
TelemetryView<Uart1Sink> view;
view.show("v=1");

TelemetryView<NullSink> off;
off.show("消える");          // 呼び出しごと最適化で消える
```

- **vtable ゼロ、ヒープゼロ、仮想呼び出しゼロ。** 全部インライン展開されます
- `sizeof(TelemetryView<Uart1Sink>)` も `sizeof(TelemetryView<NullSink>)` も **1**
  （空クラスなので実測 1 バイト）。ポインタすら持ちません
- `NullSink` 版は `put_line` が空なので、**呼び出しごと消えます**

代償は「実装を実行時に切り替えられない」ことです。
**マイコンでは、実装は基板を焼いた時点で決まっていることがほとんど**です。切り替える必要がありません。

`this->sink_` の `this->` を忘れると
`error: use of undeclared identifier 'sink_'` になります。
基底がテンプレート引数に依存しているとき、C++ は基底のメンバを自動では探しません。
テンプレートで Bridge を書くときは必ず踏みます。

**マイコンでの優先順位**: (c) テンプレート → (a) ヘッダに書く → (b) fast-pimpl。
`unique_ptr` 版の Bridge を使うのは、**実装を実行時に切り替える必然性があるときだけ**です。

## 9.11 ROS 2 での結論（補足）

rclcpp は Pimpl を多用します。`rclcpp::Node` を見てください。
`NodeBaseInterface` / `NodeTopicsInterface` などの `shared_ptr` メンバに委譲していて、
ヘッダから中身が見えないようになっています。

理由はまさに 9.6 です。**rclcpp は共有ライブラリとして配られる**ので、
ヘッダのクラスサイズが変わると利用者側の再ビルドが必要になります。

`rclcpp::Publisher` / `Subscription` は Bridge に近い形です。
「メッセージを配る」という機能側と、rmw（DDS 実装）という実装側が分かれていて、
**同じ `publish()` が Fast DDS でも Cyclone DDS でも動きます**。
実装の選択は `RMW_IMPLEMENTATION` 環境変数で実行時に決まります。
これは 9.3 の M×N を潰した典型例です。

自分でノードを書くときに Pimpl が必要になることは、まずありません。
**パッケージ内でしか使わないヘッダに Pimpl を入れるのは、9.6 の代償だけを払う行為です。**

## 9.12 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: invalid application of 'sizeof' to an incomplete type 'X::Impl'` | デストラクタをヘッダで定義（または宣言し忘れ）している。宣言だけヘッダ、`= default` は .cpp |
| `static assertion failed: can't delete an incomplete type` | 同上（GCC の文言） |
| Pimpl クラスを `std::vector` に入れようとしたら「ムーブできない」 | デストラクタを宣言したのでムーブが暗黙生成されない。ムーブも 2 行足す |
| ムーブ代入だけヘッダに書いたらまたエラー | ムーブ代入は古い `impl_` を `delete` する。これも .cpp |
| `impl_->...` で落ちる | コンストラクタで `make_unique<Impl>()` を書き忘れ。またはムーブ済みのオブジェクトを使っている |
| 機能側の基底ポインタで解放したらメンバが漏れた | 機能側の仮想デストラクタが無い。Bridge は機能側も継承される |
| 機能側のクラスが `M×N` に増えた | 実装を継承で表現している。メンバ（橋）に変える |
| `error: use of undeclared identifier 'sink_'`（テンプレート版） | 依存基底のメンバは `this->sink_` と書く |
| `unique_ptr` をコンストラクタに渡したらコンパイルエラー | `std::move` を書き忘れ。`unique_ptr` はコピーできない |
| Pimpl にしたのにビルドが速くならない | ヘッダ側でまだ `<vector>` などを include している。include を .cpp に移す |

## 9.13 対応する課題

```bash
./drill run dp09
```

`exercises/dp09_bridge/src/telemetry_view.cpp` に **Bridge** を実装します。

1. `RecordingSink` / `NumberedSink` — 実装のクラス階層
2. `TelemetryView::show()` — 機能のクラス階層。`open()` → `put_line()` → `close()`
3. `RepeatView::show_repeat()` — 機能を 1 つ増やす。**実装側は変えない**

`exercises/dp09_bridge/src/link_stats.cpp` に **Pimpl** を実装します。

4. `struct LinkStats::Impl` とコンストラクタ
5. `add_sample()` / `count()` / `mean()` / `max()`

テストが見るのは次の 4 点です。

- 機能 2 種 × 実装 2 種の **4 通りが全部動く**
- 実装を差し替えても、**機能側を呼ぶ関数は同じ 1 つ**（`run_show`）のまま
- `sizeof(LinkStats)` が**ポインタ 1 個分**であること（`static_assert`）
- `LinkStats` が**ムーブ可能でコピー不可**であること（`static_assert`）

`~LinkStats()` とムーブの定義は最初から書いてあります。**消さないでください。**
消すとどうなるかは 9.5 で確かめたとおりです。

## 9.14 この章のまとめ

- Bridge の構造は Java 版とほぼ同じ。**機能側にも仮想デストラクタが要る**のが差
- 実装への橋は `std::unique_ptr<Impl>`。**継承ではなくメンバ**。だから M×N が M+N になる
- **Bridge と Pimpl は同じ構造。目的が違う**。差し替えたいなら仮想、隠したいだけなら非仮想
- `unique_ptr<不完全型>` を持つと、**デストラクタをヘッダに書けない**。
  宣言だけヘッダ、`= default` は .cpp
- デストラクタを宣言すると**ムーブが暗黙生成されなくなる**。ムーブも 2 行足す
- Pimpl の値打ちは**ヘッダ依存が減ること**。共有ライブラリなら ABI も守れる。
  代償はヒープ 1 回と間接参照 1 段
- マイコンでは **(c) テンプレート → (a) ヘッダに書く → (b) fast-pimpl** の順で検討する。
  `unique_ptr` 版は「実行時に切り替える必然性があるとき」だけ
- **実装が 1 種類しかないなら Bridge は入れない**

---

前: [8. Abstract Factory](08_AbstractFactory.md) ／ 次: 10. Strategy（準備中）
