# 2. Adapter

> **結城本 第2章 対応。** `Banner` / `PrintBanner` の 2 つのサンプル
> （継承を使ったもの・委譲を使ったもの）を手元に開いてください。
>
> **この章のねらい**: 結城本は Adapter を **継承版と委譲版の 2 通り**で書いています。
> Java では「どちらもアリ、好みで選ぶ」に近い書き方ができます。
> **C++ では違います。** 継承版を選ぶと、菱形継承・名前衝突・スライシングが一度に降ってきます。
> この章は「**なぜ C++ では委譲版一択なのか**」を、実際のコンパイルエラーで確認する回です。
> ついでに、**`std::stack` と `std::queue` が委譲版 Adapter そのもの**であることを見ます。

## 2.1 Java 版をそのまま C++ にすると

結城本の登場人物は 3 人です。

| 役 | 意味 | この章の題材 |
| --- | --- | --- |
| Target | 使う側が見たい形 | `MotorActuator`（単位は rad/s, rad） |
| Adaptee | 既にあって変更できないもの | `LegacyMotorDriver`（単位はパルス, エンコーダカウント） |
| Adapter | 間を埋める人 | これから書く |

Java 版の委譲 Adapter はこう書かれています（結城本の `PrintBanner` に相当）。

```java
public class PrintBanner extends Print {
    private Banner banner;
    public PrintBanner(String string) {
        this.banner = new Banner(string);
    }
    public void printWeak() { banner.showWithParen(); }
}
```

C++ に素直に移すとこうです。

```cpp
class DelegatingMotorAdapter : public MotorActuator
{
public:
  explicit DelegatingMotorAdapter(LegacyMotorDriver driver)
  : driver_(std::move(driver))
  {
  }

  void set_velocity(double rad_per_sec) override;
  void stop() override;
  double position_rad() const override;

private:
  LegacyMotorDriver driver_;
};
```

Java 版から変えた点が 4 つあります。**どれも変えないとバグるか、遅くなります。**

### 変更点1: Target を `interface` ではなく純粋仮想クラスにした

結城本の `Print` は抽象クラス版と `interface` 版の両方が出てきます。
C++ には `interface` キーワードがありません。純粋仮想関数だけを持つクラスで代用します。

```cpp
class MotorActuator
{
public:
  virtual ~MotorActuator() = default;
  virtual void set_velocity(double rad_per_sec) = 0;
  virtual void stop() = 0;
  virtual double position_rad() const = 0;
};
```

**Java の `interface` と、C++ の純粋仮想クラスは同じものではありません。**

| | Java `interface` | C++ 純粋仮想クラス |
| --- | --- | --- |
| データメンバ | 持てない（定数のみ） | **持てる** |
| 実装済みメソッド | `default` メソッドのみ | いくらでも書ける |
| 多重に実装 | いくらでもできる | 多重継承になる。菱形の問題が出る |
| デストラクタ | 概念が無い | **明示的に `virtual` が要る** |

「持てる」が曲者です。C++ では Target にうっかりデータメンバを足せてしまい、
足した瞬間に多重継承の重さが変わります。**Target は空にしておく**、と決めてください。

### 変更点2: `virtual ~MotorActuator() = default;` を足した

第1章と同じ話です。書き忘れると、基底ポインタで消したときに派生が消えません。
コンパイラは警告してくれます。

```cpp
struct MotorActuator { virtual void stop() = 0; };   // 仮想デストラクタを書き忘れた
struct Adapter : MotorActuator { void stop() override {} };

MotorActuator * m = new Adapter{};
delete m;
```

```
warning: delete called on 'MotorActuator' that is abstract but has
non-virtual destructor [-Wdelete-abstract-non-virtual-dtor]
```

**警告であって、エラーではありません。** `-Wall` を切っている環境では黙って通ります。
純粋仮想関数を 1 つでも書いたら仮想デストラクタ、を毎回やってください。

### 変更点3: Adaptee を**値で持った**

Java 版は `private Banner banner;` です。参照が入っています。
C++ で同じ気分で書くとこうなりがちです。

```cpp
LegacyMotorDriver & driver_;      // 参照。所有しない
LegacyMotorDriver * driver_;      // 生ポインタ。誰が消すのか不明
LegacyMotorDriver driver_;        // 値。Adapter が所有する ← この課題ではこれ
```

**3 つとも正解になり得ます。決めるのは「Adaptee が誰のものか」です。**

| 持ち方 | 意味 | いつ |
| --- | --- | --- |
| 値 | Adapter が Adaptee を所有する | Adaptee がこの Adapter 専用。**既定はこれ** |
| 参照 | 外にある Adaptee を借りる | ペリフェラルなど、世界に 1 個しかないもの |
| `unique_ptr` | 所有するが、多態が要る / 型を隠したい | Adaptee 側にも継承がある |
| `shared_ptr` | 複数の Adapter が同じ Adaptee を共有 | 本当に共有が要るときだけ |

**Java にはこの選択肢がありません**（常に参照）。C++ で Adapter を書くとき、
最初に決めるのはパターンの形ではなく**この行**です。

### 変更点4: `position_rad()` に `const` を付けた

読むだけの関数は `const` メンバ関数にします。Java にこの区別はありません。
`const MotorActuator &` で受け取ったとき、`const` の付いていない関数は呼べません。

## 2.2 誰が所有するのか

上位のコードはこう書きます。

```cpp
std::vector<std::unique_ptr<MotorActuator>> motors;
motors.push_back(std::make_unique<DelegatingMotorAdapter>(LegacyMotorDriver{}));
```

`MotorActuator` は抽象クラスなので**値では持てません**。多態が要る以上、
`unique_ptr` の `vector` になります。ここは第1章と同じ結論です。

Adaptee の寿命は、上の表で「値」を選んだ時点で解決しています。
**Adapter が死ねば Adaptee も死ぬ。** それ以上考えることがありません。

参照で持った場合だけ、第1章と同じ寿命の問題が残ります。

```cpp
std::unique_ptr<MotorActuator> make_motor()
{
  LegacyMotorDriver driver;                     // ローカル変数
  return std::make_unique<BorrowingAdapter>(driver);   // driver はここで死ぬ
}
```

Java なら GC が `driver` を生かします。C++ では死にます。
**参照で持つと決めたら、「Adaptee は Adapter より長生きさせること」をコメントに書く**のが仕事です。

## 2.3 継承版を C++ で書くと何が起きるか

結城本の継承版はこうです。

```java
public class PrintBanner extends Banner implements Print {
    public void printWeak() { showWithParen(); }
}
```

`extends`（実装の継承）と `implements`（インタフェースの実装）が
**キーワードで区別されています**。C++ にはその区別がありません。両方 `:` です。

```cpp
class InheritingMotorAdapter : public MotorActuator, private LegacyMotorDriver
```

これは**多重継承**です。Java の `extends Banner implements Print` は
「1 クラス + インタフェース」なので多重継承ではありませんが、**C++ では多重継承になります。**
ここから 4 つの問題が出ます。

### 問題1: 菱形継承

Target 側のインタフェースが 2 本になった瞬間に起きます。

```cpp
struct Device { virtual ~Device() = default; virtual void reset() = 0; };
struct Readable : Device { virtual double read() const = 0; };
struct Writable : Device { virtual void write(double v) = 0; };

struct Adapter : Readable, Writable
{
  void reset() override {}
  double read() const override { return 0.0; }
  void write(double) override {}
};

Adapter a;
Device * d = &a;
```

```
error: ambiguous conversion from derived class 'Adapter' to base class 'Device':
    struct Adapter -> Readable -> Device
    struct Adapter -> Writable -> Device
```

`Adapter` の中に `Device` が **2 つ**入っています。基底ポインタに変換できません。
直すには `Readable : virtual Device` と `Writable : virtual Device` にします
（**仮想継承**）。すると今度は、

- `Adapter` のオブジェクトに**仮想基底ポインタ**が増える（サイズが太る）
- 基底へのアクセスが**間接参照 1 段**増える
- 最派生クラスが仮想基底のコンストラクタを直接呼ぶ、という初期化規則が生える

**Java にはこの問題が一切ありません。** インタフェースは実装を持たないので、
何本重ねてもデータが重複しません。「Java でできるから C++ でも」が通じない典型です。

### 問題2: 名前衝突

Target と Adaptee に同じ名前があると、**呼んだ瞬間にエラー**です。

```cpp
struct MotorActuator { virtual ~MotorActuator() = default;
                       virtual void set_velocity(double) = 0; void reset() {} };
struct LegacyDriver { void reset() {} };
struct Adapter : MotorActuator, private LegacyDriver
{
  void set_velocity(double) override {}
};

Adapter a;
a.reset();
```

```
error: member 'reset' found in multiple base classes of different types
note: member found by ambiguous name lookup
```

Adaptee は**変更できない前提**です。名前が当たったら、Adapter 側で
`using LegacyDriver::reset;` などを書いて曖昧さを潰すしかありません。
**Adaptee にメンバが増えるたびに、こちらが壊れる可能性がある。** これが継承版の維持コストです。

さらに悪いのは、**エラーにならない衝突**です。

```cpp
struct MotorActuator { virtual void reset() = 0; /* ... */ };
struct LegacyDriver { void reset() {} };

struct Adapter : MotorActuator, private LegacyDriver
{
  void reset() override { reset(); }   // Adaptee を呼んだつもりが、自分を呼んでいる
};
```

無修飾の `reset()` は**自分自身**に解決されます。無限再帰です。コンパイルは通ります。
正しくは `LegacyDriver::reset();` と書きます。委譲版なら `driver_.reset();` で、
**そもそも間違えようがありません。**

### 問題3: `public` 継承にするとスライシングが通る

「`private` 継承なんて面倒だから `public` で」とやると、こうなります。

```cpp
struct LegacyDriver { int pulse = 0; void setPulse(int p) { pulse = p; } };
struct PublicAdapter : public LegacyDriver { double gain = 2.0; };
struct PrivateAdapter : private LegacyDriver { double gain = 2.0; };

void tune(LegacyDriver driver) { driver.setPulse(0); }   // 値渡し

PublicAdapter pub;
tune(pub);            // 通る。gain が切り落とされる

PrivateAdapter priv;
tune(priv);           // 通らない
```

`private` 版だけエラーになります。

```
error: cannot cast 'const PrivateAdapter' to its private base class 'const LegacyDriver'
note: declared private here
```

**`public` 版は黙って通ります。警告すら出ません。**
`gain` が切り落とされたコピーが作られ、以後の変更は元に届きません。
Java に値のコピーが無いので、**スライシングは Java 版を読んでいるだけでは絶対に気付けない**危険です。

そして `public` 継承版には、もっと素直な問題があります。

```cpp
MotorActuator * m = get_motor();
static_cast<PublicAdapter *>(m)->setPulse(999);   // 生の単位で直接叩ける
```

**単位を隠すという Adapter の目的が、public 継承で崩れています。**

### 問題4: Adaptee に仮想デストラクタが無い

`LegacyMotorDriver` は 3 年前のコードです。仮想デストラクタなんてありません。
public 継承すると「`LegacyMotorDriver *` に代入できるのに、そこで消すと壊れる」型ができます。
`private` 継承なら、そもそも代入できないので事故りません。

### 結論: 委譲版一択

| | 継承版（private） | 委譲版 |
| --- | --- | --- |
| 菱形継承 | Target が 2 本になると発生 | 起きない |
| 名前衝突 | Adaptee が変わると壊れうる | 起きない |
| Adaptee の呼び出し | 無修飾だと自分を呼ぶ事故 | `driver_.` で明示 |
| Adaptee を実行時に差し替える | できない | メンバを入れ替えれば可能 |
| 1 つの Adapter で 2 つの Adaptee を束ねる | できない（両方継承は地獄） | メンバを 2 つ持つだけ |
| Adaptee の protected メンバを使う | **使える** | 使えない |
| オブジェクトサイズ | 空基底最適化が効くことがある | Adaptee 分そのまま |

**継承版が勝つのは下 2 行だけです。** そして
「Adaptee の `protected` を使いたい」は、**変更できない他人のライブラリでは滅多に起きません**。

> **迷ったら委譲。** 継承を選ぶのは、`protected` メンバが要ると分かったときだけ。

### private 継承という C++ 固有の選択肢

Java に `private extends` はありません。C++ の `private` 継承は
**"is-a"（〜である）ではなく "is-implemented-in-terms-of"（〜を使って実装されている）** を表します。

```cpp
class InheritingMotorAdapter : public MotorActuator, private LegacyMotorDriver
```

これは「`MotorActuator` **である**。そして `LegacyMotorDriver` を**使って実装されている**」と読みます。
`private` 継承には、委譲に無い小技が 1 つあります。**選択的な再公開**です。

```cpp
using LegacyMotorDriver::getPulse;    // これだけ外に出す
```

課題ではこれを使ってテストから中を覗きます。
とはいえ、委譲版でも「その 1 行を転送する関数を書く」だけで同じことができます。
**private 継承は "書く量がわずかに減る" 以上の価値をほとんど持ちません。**

## 2.4 標準ライブラリに同じものが無いか

あります。しかも**毎日使っているもの**です。

```cpp
std::stack<int>    // 既定の中身は std::deque<int>
std::queue<int>    // 既定の中身は std::deque<int>
```

`std::stack` は自分でデータを持ちません。`std::deque` を**メンバとして持ち**、
`push_back` / `pop_back` / `back` を `push` / `pop` / `top` に読み替えて公開しているだけです。
規格でも **container adaptor** と呼ばれています。

| Adapter の役 | `std::stack` では |
| --- | --- |
| Target | 「スタック」という口（`push` / `pop` / `top`） |
| Adaptee | `std::deque`（既定） |
| Adapter | `std::stack` 本体 |

**そして `std::stack` は委譲版です。** `std::deque` を継承していません。
標準ライブラリが委譲を選んだ理由は、この章でここまで見てきたものと同じです。

`std::stack` の第 2 テンプレート引数で、Adaptee を丸ごと差し替えられます。

```cpp
std::stack<int, std::vector<int>> vector_stack;   // 中身を vector にする
```

**Adaptee を差し替えられるのは委譲だから**です。継承していたら型が変わってしまいます。

そしてもう 1 点。Adapter は**口を絞る**道具でもあります。

```cpp
std::deque<int> raw;
raw.push_front(0);          // deque にはある

std::stack<int> s;
// s.push_front(0);         // stack には無い
// for (int v : s) { }      // 走査もできない
```

`std::stack` は `std::deque` より**できることが少ない**。これが仕事です。
課題の `MotorActuator` も同じで、「パルスで叩く」という口を**わざと消して**います。

## 2.5 手元で試す

課題を解く前に、この 1 ファイルをコンパイルして**出力を予想してから**実行してください。

```cpp
#include <deque>
#include <iostream>
#include <stack>
#include <vector>

int main()
{
  // std::stack は「コンテナを 1 つ持って、外に出す口を絞る」だけの委譲版 Adapter。
  // 既定の中身は std::deque。
  std::stack<int> default_stack;

  // 中身を差し替えても、外から見た口はまったく同じ。
  std::stack<int, std::vector<int>> vector_stack;

  for (int i = 1; i <= 3; ++i) {
    default_stack.push(i);
    vector_stack.push(i);
  }

  std::cout << default_stack.top() << " " << vector_stack.top() << "\n";
  std::cout << default_stack.size() << " " << vector_stack.size() << "\n";

  // Adaptee をそのまま使うと、口は絞られていない。
  std::deque<int> raw;
  raw.push_back(1);
  raw.push_front(0);              // stack には無い操作
  std::cout << raw.front() << " " << raw.back() << "\n";

  // std::stack にできないこと: 走査
  // for (int v : default_stack) { (void)v; }   // ← コメントを外すとエラーになる
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 3 行の出力は何か。そして最後のコメントを外すと何が起きるか</summary>

```
3 3
3 3
0 1
```

1 行目・2 行目が**同じ**なのが要点です。`std::deque` を中身にしても
`std::vector` を中身にしても、**外から見た口は 1 文字も変わりません**。
Adaptee を差し替えても Target が変わらない、これが Adapter です。

最後のコメントを外すとこうなります。

```
error: invalid range expression of type 'std::stack<int>'; no viable 'begin' function available
```

`std::deque` には `begin()` があります。`std::stack` はそれを**公開していません**。
Adapter は口を広げる道具であると同時に、**絞る道具**でもあると分かります。
</details>

## 2.6 マイコンでの結論

**継承版は論外、委譲版もそのままでは使いません。** 理由は 3 つです。

1. `MotorActuator` を仮想関数で作ると、**vtable が ROM に、vtable ポインタが各オブジェクトに**乗る
2. `std::unique_ptr<MotorActuator>` で持つと**ヒープ確保**が要る
3. 呼び出しがインライン展開されない

vtable ポインタのコストは実測できます。手元の arm64 環境で、

| 型 | `sizeof` |
| --- | --- |
| `LegacyMotorDriver`（`int` + `int32_t`） | 8 |
| 仮想関数を持つ委譲 Adapter | **16** |
| テンプレート版の委譲 Adapter | 8 |

**オブジェクト 1 個あたり 8 バイト**増えます。モータ 4 個なら 32 バイト。
RAM が 20 KB のマイコンで、これを 20 箇所でやると効いてきます。

代わりに、**型ではなく名前で揃えます**。テンプレートの委譲 Adapter です。

```cpp
#include <cstdint>

// 既存の生ドライバ（変更不可）
class LegacyMotorDriver
{
public:
  void setPulse(int pulse) { pulse_ = pulse; }
  int getPulse() const { return pulse_; }
  void stopAll() { pulse_ = 0; }
  std::int32_t readEncoderRaw() const { return encoder_raw_; }

private:
  int pulse_ = 0;
  std::int32_t encoder_raw_ = 0;
};

// 仮想関数なし・継承なし・確保なしの委譲版 Adapter。
// 「MotorActuator という型」ではなく「set_velocity という名前」で揃える。
template <typename Driver>
class MotorAdapter
{
public:
  explicit MotorAdapter(Driver & driver) : driver_(driver) {}

  void set_velocity(std::int32_t milli_rad_per_sec)
  {
    driver_.setPulse(static_cast<int>(milli_rad_per_sec / 10));
  }

  void stop() { driver_.stopAll(); }

private:
  Driver & driver_;              // 所有しない。寿命は呼び出し側が保証する
};

// 使う側もテンプレート。呼び出しは全部インライン展開される
template <typename Motor>
void drive_forward(Motor & motor)
{
  motor.set_velocity(1000);
}

LegacyMotorDriver g_driver;        // 静的記憶域。起動時に 1 回だけ構築される

int main()
{
  MotorAdapter<LegacyMotorDriver> motor{g_driver};
  drive_forward(motor);
  motor.stop();
  return g_driver.getPulse();
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti micro.cpp -o micro
```

マイコンの制約に対して、それぞれこうなっています。

| 制約 | この書き方でどうなるか |
| --- | --- |
| 動的確保 原則禁止 | `new` も `make_unique` も無い。Adaptee は静的記憶域、Adapter はスタック |
| `-fno-exceptions` | `throw` していない。エラーは戻り値で返す |
| `-fno-rtti` | `dynamic_cast` も `typeid` も無い。そもそも継承していない |
| vtable コスト | 仮想関数ゼロ。vtable も vtable ポインタも生えない |

**代償は「実行時に Adaptee を差し替えられない」ことだけ**です。
マイコンで、繋がっているモータドライバの型が実行時に変わることはありません。
**コンパイル時に決まっているものは、コンパイル時に決めます。**

単位の変換にも 1 つ注意があります。上のコードは `double` を使わず
`milli_rad_per_sec`（`std::int32_t`）で受けています。
FPU の無いマイコンでは `double` の演算がソフトウェアエミュレーションになり、
**割り込みハンドラの中で数百サイクル**持っていかれます。
**Adapter は単位を変える場所なので、ここが浮動小数点の入口になりがちです。**
固定小数点で受けるか、少なくとも `float` にしてください。

どうしても実行時の差し替えが要るなら（テスト用のダミードライバと入れ替えたい、など）、
仮想関数版を使い、`unique_ptr` ではなく**静的記憶域に置いた実体を指す**形にします。

## 2.7 ROS 2 での結論（補足）

ROS 2 側では、2.1〜2.4 の委譲版をそのまま書いて構いません。
確保も仮想関数もコストとして気にする場面はほとんどありません。

rclcpp まわりで Adapter が実際に効くのは、**ハードウェア層とメッセージ層の境目**です。

```cpp
// 生ドライバは pulse を知っている。ノードは geometry_msgs だけ知っていればいい
class MotorAdapter : public MotorActuator { /* ... */ };
```

ノードのコールバックが `LegacyMotorDriver` を直接触っていると、
**ドライバを差し替えた日にノードのテストが全部書き直し**になります。
Adapter を 1 枚挟んでおけば、差し替えは Adapter だけで済みます。

なお `rclcpp::TypeAdapter`（Humble 以降）は名前こそ Adapter ですが、
「独自型と ROS メッセージ型の相互変換をテンプレート特殊化で登録する」仕組みで、
**GoF の Adapter とは別物**です。混同しないでください。

## 2.8 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: ambiguous conversion from derived class ... to base class` | 菱形継承。Target 側のインタフェースが 2 本になっている。委譲に変える |
| `error: member 'reset' found in multiple base classes of different types` | Target と Adaptee の名前衝突。委譲なら起きない |
| 継承版で Adaptee を呼んだつもりが無限再帰する | 無修飾の呼び出しが自分に解決されている。`LegacyDriver::reset()` と書く |
| 関数に渡したら Adapter のメンバが消えていた | public 継承 + 値渡しでスライシング。`private` 継承か委譲にする |
| `warning: delete called on ... non-virtual destructor` | Target に仮想デストラクタが無い |
| Adapter を返す関数の戻り値を使うと落ちる | Adaptee を参照で持っている。値で持つか、寿命を約束する |
| `error: cannot cast ... to its private base class` | `private` 継承は外から基底に変換できない。**これは正しい動作** |
| 単位変換が呼び出しごとに違う値になる | 変換係数が Adapter の外に散っている。Target 側に `constexpr` で 1 箇所に置く |

## 2.9 対応する課題

```bash
./drill run dp02
```

`exercises/dp02_adapter/src/motor_adapter.cpp` に、

1. `DelegatingMotorAdapter` — 生ドライバを**メンバとして持つ**委譲版
2. `InheritingMotorAdapter` — 生ドライバを **`private` 継承**する継承版

の 2 つを実装します。テストは**両方がまったく同じ振る舞いになること**と、
`std::unique_ptr<MotorActuator>` の `vector` に混ぜて多態に扱えることを見ます。

書き終えたら、`InheritingMotorAdapter` の `private` を `public` に変えて
何が起きるか（何が**起きないか**）を確かめてください。**それが 2.3 の答え合わせです。**

## 2.10 この章のまとめ

- Java の `extends` + `implements` は、**C++ では多重継承**になる
- 継承版は、菱形継承・名前衝突・スライシング・仮想デストラクタ欠如を一度に呼び込む
- **委譲版一択。** 継承が勝つのは Adaptee の `protected` が要るときだけ
- `private` 継承は "is-implemented-in-terms-of"。**Java に無い選択肢**だが、委譲で足りる
- Java の `interface` と C++ の純粋仮想クラスは別物。**Target は空に保つ**
- Adapter を書く前に決めるのは形ではなく、**Adaptee を値 / 参照 / `unique_ptr` のどれで持つか**
- **`std::stack` と `std::queue` は委譲版 Adapter**。中身を差し替えられるのは委譲だから
- Adapter は口を広げるだけでなく**絞る**道具。`std::stack` に `begin()` は無い
- マイコンでは仮想関数を捨ててテンプレートの委譲にする。**vtable ポインタ 8 バイト / オブジェクト**が浮く

---

前: [1. Iterator](01_Iterator.md) ／ 次: [3. Template Method](03_TemplateMethod.md)
