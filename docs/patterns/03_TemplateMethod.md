# 3. Template Method

> **結城本 第3章 対応。** `AbstractDisplay` と `CharDisplay` / `StringDisplay` を手元に開いてください。
>
> **この章のねらい**: 結城本の `AbstractDisplay` は
> **`public final void display()` が `protected abstract` な `open` / `print` / `close` を呼ぶ**構造です。
> C++ に移すと、この 1 行に 3 つの差が同時に現れます。
> `final` は「`virtual` を書かない」に、`protected abstract` は「`private` な純粋仮想関数」に変わり、
> **仮想デストラクタ**が新たに必要になります。
> さらに C++ には Java に無い落とし穴が 1 つあります。
> **コンストラクタから仮想関数を呼ぶと派生の実装が呼ばれません。**
> この形は **NVI (Non-Virtual Interface) イディオム** と呼ばれ、
> Template Method の C++ での標準的な書き方です。

## 3.1 Java 版をそのまま C++ にすると

結城本の `AbstractDisplay` はこうです。

```java
public abstract class AbstractDisplay {
    public abstract void open();
    public abstract void print();
    public abstract void close();
    public final void display() {
        open();
        for (int i = 0; i < 5; i++) {
            print();
        }
        close();
    }
}
```

C++ に素直に移すとこうなります。

```cpp
class AbstractDisplay
{
public:
  virtual ~AbstractDisplay() = default;

  void display()                 // ← 骨格。virtual を付けない
  {
    open();
    for (int i = 0; i < 5; ++i) {
      print();
    }
    close();
  }

private:                         // ← Java は protected。C++ では private でよい
  virtual void open() = 0;
  virtual void print() = 0;
  virtual void close() = 0;
};
```

変えた点が 3 つあります。**どれも意味があります。**

### 変更点1: `virtual ~AbstractDisplay() = default;` を足した

第 1 章と同じです。**基底クラスのポインタで破棄するなら、デストラクタは仮想。**
Template Method は「基底の型で持って、派生の中身を呼ぶ」パターンなので、
必ず基底ポインタ経由の破棄が起きます。書き忘れると静かにリークします。

詳しくは [C++編 3.5 仮想デストラクタ](../cpp/03_継承.md) を見てください。

### 変更点2: `final void display()` → `void display()`（`virtual` を書かない）

Java のメソッドは**書かなくても仮想**です。だから「差し替えさせない」ときに `final` を付けます。
C++ は逆で、**`virtual` を書かなければ最初から仮想ではありません。**

| | 差し替えられる | 差し替えられない |
| --- | --- | --- |
| Java | `void f()`（既定） | `final void f()` |
| C++ | `virtual void f()` | `void f()`（既定） |

**既定が逆です。** ここが Java から来た人が最初に事故るところで、事故り方は 2 通りあります。

```cpp
// 事故1: 骨格まで virtual にしてしまう
virtual void display() { ... }   // 派生が丸ごと差し替えられる = 骨格を守れない

// 事故2: 差し替えてほしい部分に virtual を書き忘れる
void open();                     // 派生が open() を書いても呼ばれない。静かに基底が動く
```

事故 2 は**コンパイルが通ります**。`AbstractDisplay *` 経由で `display()` を呼ぶと
基底の `open()` が黙って動きます。Java では起きません。

`display()` に「差し替え禁止」を明示したいなら、**コメントで書く**しかありません。
`virtual` が無いこと自体が「差し替え禁止」の宣言だからです。

### 変更点3: `protected abstract` → `private virtual`

ここが Java から来ると一番信じられないところです。

> **C++ では、`private` な仮想関数を派生クラスがオーバーライドできます。**

**アクセス指定とオーバーライド可否は独立**しています。`private` が禁じているのは
「その名前を呼ぶこと」だけで、「差し替えること」ではありません。

```cpp
class Sensor
{
private:
  virtual void setup() { }       // private
};

class Imu : public Sensor
{
private:
  void setup() override { }      // OK。private でもオーバーライドできる
};
```

Java の `private` メソッドはオーバーライドできない（できたように見えても別メソッドになる）ので、
結城本は `protected abstract` にしています。C++ では `private` にできて、そのほうが良い。
理由は **派生クラスに「呼ぶ」権限を渡さないから**です。

```cpp
class Imu : public Sensor
{
private:
  void setup() override
  {
    Sensor::setup();     // private なのでエラー。派生から基底版を呼ぶ道は閉じている
  }
};
```

実際に出るエラーです。

```
error: 'setup' is a private member of 'Sensor'
note: declared private here
```

派生クラスにできるのは「中身を埋めること」だけになります。
**手順の呼び出しは骨格が独占する。** これが NVI (Non-Virtual Interface) イディオムです。

| | アクセス指定 | 意味 |
| --- | --- | --- |
| 骨格 `display()` | `public` 非仮想 | 外から呼べる。差し替えられない |
| 各段 `open()` など | `private virtual` | 外からも派生からも呼べない。差し替えだけできる |
| 派生が基底版を呼びたい段 | `protected virtual` | 差し替えたうえで基底版も呼べる |

**3 行目だけ `protected`** にします。「基底の判定に条件を足す」形のフックがこれです。
課題の `validate()` がその例で、派生は `SensorReader::validate()` を呼んでから
自分の範囲判定を足します。**呼ぶ必要が無いなら `private` のまま**にしてください。

## 3.2 `override` を必ず書く

第 3 章の構造では `override` の有無が生死を分けます。

```cpp
class Sensor
{
private:
  virtual bool check(double value) const { return value == value; }
};

class Imu : public Sensor
{
private:
  bool check(double value) { return value > 0.0; }   // const を落とした
};
```

`check(double) const` と `check(double)` は**別のシグネチャ**です。
`Imu::check` はオーバーライドではなく、**まったく新しい関数**になります。
骨格から呼ばれるのは基底の `check` のままです。**コンパイルは通り、警告も出ません。**

`override` を書くと、その場でエラーになります。

```
error: non-virtual member function marked 'override' hides virtual member function
note: hidden overloaded virtual function 'Sensor::check' declared here:
      different qualifiers ('const' vs unqualified)
```

`override` は「基底の仮想関数を差し替えているつもりだ」という宣言で、
そうでなければコンパイルを止めます。**Template Method では骨格が黙って基底版を呼び続ける**
という最悪の壊れ方をするので、例外なく書いてください。

逆に、非仮想の骨格に `override` を付けるとこうなります。

```cpp
class Imu : public Sensor
{
public:
  void boot() override { }     // Sensor::boot() は非仮想
};
```

```
error: only virtual member functions can be marked 'override'
```

**これは良い間違いです。** 「骨格を差し替えようとしている」と気づけます。

## 3.3 `final` で差し替えを止める（C++11）

一段だけ実装したあと、その先の派生に差し替えさせたくないことがあります。

```cpp
class Sensor
{
private:
  virtual void setup() final { }    // ここから先は差し替え禁止
};
```

```
error: declaration of 'setup' overrides a 'final' function
note: overridden virtual function is here
```

Java の `final` メソッドと同じ効果ですが、C++ では **`virtual` の連鎖を途中で切る**ために使います。
クラスそのものに付けると継承自体が止まります。

```cpp
class EncoderReader final : public SensorReader { };   // これ以上派生させない
```

課題の `EncoderReader` / `ThermistorReader` はこの形にしてあります。
**「葉のクラスには `final` を付ける」**は、C++ では単なる意思表示以上の意味があり、
コンパイラが仮想呼び出しを直接呼び出しに置き換えられる（devirtualization）ようになります。

## 3.4 C++ 固有の危険 — コンストラクタから仮想関数を呼ぶ

**Java との差が一番はっきり出るところです。**

```cpp
class Sensor
{
public:
  Sensor()
  {
    setup();          // 危険
  }
private:
  virtual void setup() { /* 基底の実装 */ }
};
```

C++ では、**基底クラスのコンストラクタが走っている間、オブジェクトはまだ基底クラスです。**
vptr は基底の vtable を指しています。だから `setup()` は**基底の実装が呼ばれます**。
派生のオーバーライドは呼ばれません。

Java は逆で、コンストラクタからでも**派生のオーバーライドが呼ばれます**
（フィールドが未初期化のまま動くので、それはそれで有名なバグ源です）。

デストラクタでも同じことが起きます。派生の破棄が終わってから基底のデストラクタが走るので、
そこから呼ぶ仮想関数は基底の実装です。

純粋仮想関数だった場合はさらに悪く、**実行時に落ちます**。

```cpp
struct B { B() { setup(); } virtual ~B() = default; virtual void setup() = 0; };
struct D : B { void setup() override { std::printf("D\n"); } };
int main() { D d; (void)d; }
```

```
warning: call to pure virtual member function 'setup' has undefined behavior;
         overrides of 'setup' in subclasses are not available in the constructor of 'B'
         [-Wcall-to-pure-virtual-from-ctor-dtor]
```

```
libc++abi: Pure virtual function called!
```

（Apple clang 17 / libc++ での実行結果。gcc / libstdc++ では
`pure virtual method called` というメッセージになります）

**この場合は警告が出ます**が、3.4 冒頭のように基底に実装がある場合は
警告も出ずに静かに基底が呼ばれます。そちらのほうが厄介です。

**規則**: コンストラクタ／デストラクタから仮想関数を呼ばない。
初期化のうち派生に任せたい部分があるなら、**コンストラクタではなく骨格メソッドの中でやります**。
課題の `read_once()` が「初回だけ `initialize()` を呼ぶ」形になっているのはこのためです。

```cpp
std::optional<double> SensorReader::read_once()
{
  if (!is_initialized_) {
    initialize();            // ここなら派生の実装が呼ばれる
    is_initialized_ = true;
  }
  // ...
}
```

## 3.5 標準ライブラリ／言語機能に同じものが無いか

Template Method に相当する言語機能はありません。**この章は自分で書くのが正解です。**
ただし「骨格は固定、一部だけ差し替える」構造は標準ライブラリにも出てきます。

| 標準ライブラリ | 骨格 | 差し替える部分 |
| --- | --- | --- |
| `std::sort` | ソートアルゴリズム全体 | 比較（コンパレータ） |
| `std::basic_streambuf` | ストリームの入出力手順 | `overflow` / `underflow`（**protected virtual**） |

`std::basic_streambuf` は標準ライブラリの中の数少ない NVI の例です。
`pubsync()`（public 非仮想）が `sync()`（protected virtual）を呼びます。
**`pub` が付く public 関数と、付かない仮想関数の対**になっているので、
ヘッダを覗くと構造がそのまま見えます。

一方 `std::sort` は**仮想関数を 1 つも使いません**。差し替える部分をテンプレート引数で受けます。
「実行時に差し替える必要が無いなら、仮想関数を使わない」という判断で、
これが 3.7 の CRTP につながります。差し替え手段の選択そのものは
第 10 章（Strategy）の主題です。

## 3.6 手元で試す

課題を解く前に、この 1 ファイルをコンパイルして**出力を予想してから**実行してください。

```cpp
#include <iostream>

class Sensor
{
public:
  Sensor()
  {
    std::cout << "Sensor() から setup() を呼ぶ\n";
    setup();                       // 危険。派生はまだ存在していない
  }
  virtual ~Sensor() = default;

  // 骨格。virtual を付けない = 差し替えさせない
  void boot()
  {
    std::cout << "boot() から setup() を呼ぶ\n";
    setup();
  }

private:
  // private なのに派生クラスはオーバーライドできる
  virtual void setup() { std::cout << "  Sensor::setup\n"; }
};

class Imu : public Sensor
{
private:
  void setup() override { std::cout << "  Imu::setup\n"; }
};

int main()
{
  Imu imu;
  imu.boot();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: <code>setup()</code> は 2 回呼ばれる。2 回とも <code>Imu::setup</code> か</summary>

**違います。1 回目は `Sensor::setup` です。**

```
Sensor() から setup() を呼ぶ
  Sensor::setup
boot() から setup() を呼ぶ
  Imu::setup
```

コンストラクタが走っている間、オブジェクトはまだ `Sensor` です。
**Java なら 2 回とも `Imu::setup` が呼ばれます。** ここが最大の差です。

ついでに 2 つ確認できます。

- `private virtual` な `setup()` を `Imu` が `override` できている
- `boot()` は非仮想なので、`Imu` 側で差し替える手段が無い

`Imu` に `void boot() override { }` を足してみてください。

```
error: only virtual member functions can be marked 'override'
```

**骨格は守られています。**
</details>

## 3.7 マイコンでの結論

**仮想関数版でも書けます。** Template Method は Iterator と違って
**ループの中で動的確保が起きません**。基底クラスのポインタで持つ必要すらなく、
`EncoderReader encoder{...};` と実体を静的に置けます。

```cpp
static EncoderReader encoder{...};      // グローバル or static。確保はゼロ
encoder.read_once();                    // 呼び出しは仮想 3〜4 回
```

`-fno-exceptions` / `-fno-rtti` とも衝突しません。仮想関数に例外も RTTI も要らないからです。
戻り値は `std::optional<double>` のままで構いません（`optional` は確保しません）。
**`throw` で失敗を伝えないこと**だけ守ってください。

残るコストは 2 つです。

1. オブジェクトごとに vptr が 8 バイト（32bit マイコンなら 4 バイト）
2. クラスごとに vtable が ROM に載る
3. 1 回の読み取りにつき仮想呼び出しが 3〜4 回。インライン展開されない

**センサが 3 個で 1kHz なら誤差です。書いてください。**
問題になるのは「派生が数十種類ある」「読み取りが割り込みハンドラの中にある」場合です。

### vtable ゼロで同じ構造を作る — CRTP

差し替えが**コンパイル時に決まっている**なら、仮想関数は要りません。
基底クラスを「派生クラスの型」でテンプレート化します。
**CRTP (Curiously Recurring Template Pattern)** と呼ばれる形です。

```cpp
#include <cstdint>
#include <cstdio>

// CRTP 版 Template Method。vtable なし、動的確保なし、例外なし
template <typename Derived>
class SensorReaderBase
{
public:
  // 骨格。virtual は 1 つも無い
  bool read_once(std::int32_t * out)
  {
    Derived & self = static_cast<Derived &>(*this);
    if (!is_initialized_) {
      self.initialize();
      is_initialized_ = true;
    }
    const std::int32_t raw = self.fetch_raw();
    const std::int32_t value = self.convert(raw);
    if (!self.validate(value)) {
      return false;
    }
    *out = value;
    return true;
  }

protected:
  // 既定のフック。派生が同名を定義すれば、そちらが選ばれる
  bool validate(std::int32_t) const { return true; }

private:
  bool is_initialized_ = false;
};

class AdcThermistor : public SensorReaderBase<AdcThermistor>
{
public:
  void initialize() { index_ = 0; }
  std::int32_t fetch_raw() { return samples_[index_++ % 3]; }
  std::int32_t convert(std::int32_t raw) const { return raw / 10 - 20; }
  bool validate(std::int32_t celsius) const { return celsius >= -10 && celsius <= 120; }

private:
  std::int32_t samples_[3] = {200, 400, 2000};
  unsigned index_ = 0;
};

int main()
{
  AdcThermistor sensor;
  for (int i = 0; i < 3; ++i) {
    std::int32_t value = 0;
    if (sensor.read_once(&value)) {
      std::printf("ok %d\n", value);
    } else {
      std::printf("rejected\n");
    }
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti crtp.cpp -o crtp && ./crtp
```

```
ok 0
ok 20
rejected
```

**骨格が 1 か所にある**という Template Method の目的はそのまま達成されていて、
`virtual` が 1 つもありません。`static_cast<Derived &>(*this)` で「自分は本当は派生だ」と
コンパイラに教えているだけです。

サイズも減ります。同じメンバ（`bool` 1 つと `unsigned` 1 つ）を持つクラスで比べます。

```cpp
#include <cstdio>

template <typename Derived>
struct CrtpBase { bool is_initialized_ = false; };
struct CrtpSensor : CrtpBase<CrtpSensor> { unsigned index_ = 0; };

struct VirtualBase { virtual ~VirtualBase() = default; bool is_initialized_ = false; };
struct VirtualSensor : VirtualBase { unsigned index_ = 0; };

int main()
{
  std::printf("CRTP    : %zu\n", sizeof(CrtpSensor));
  std::printf("virtual : %zu\n", sizeof(VirtualSensor));
  return 0;
}
```

```
CRTP    : 8
virtual : 16
```

**vptr の 8 バイトと、アラインメントの分がまるごと消えています。**（arm64 / Apple clang 17 で計測）
呼び出しもすべてインライン展開の候補になります。

CRTP の代償は 2 つです。

| 失うもの | 内容 |
| --- | --- |
| 実行時の多態 | `std::vector<SensorReaderBase *>` に混ぜられない。型ごとに別のリストになる |
| エラーメッセージ | 段の実装を書き忘れたときのエラーがテンプレートの中で出る。読みにくい |

**判断はこうです。**

| 状況 | 選ぶもの |
| --- | --- |
| センサの種類がコンパイル時に決まっている（部活のロボットはほぼこれ） | **CRTP** |
| 実行時に種類を選ぶ／全センサを 1 本のリストで回したい | 仮想関数 + NVI |
| 迷っている | **仮想関数 + NVI から始める。** 測ってから CRTP に移す |

**課題は仮想関数版で書きます。** CRTP から入ると「何が起きているか」が見えにくく、
Java 版との対応も取れなくなるためです。

## 3.8 ROS 2 での結論（補足）

ROS 2 側では素直に仮想関数版を使ってください。vtable のコストを気にする場面はありません。

rclcpp 周辺の NVI らしい例は `rclcpp_lifecycle::LifecycleNode` です。
`configure()` / `activate()` を呼ぶと、遷移の骨格（状態チェック、状態の更新、通知）は
ライフサイクルの実装が持ったまま、`on_configure()` / `on_activate()` という
**差し替え用のコールバックだけ**が呼ばれます。
`on_configure()` は `virtual` ですが、`configure()` は差し替えの対象ではありません。
第 3 章の構造そのままです。

`hardware_interface::SystemInterface` の `on_init()` / `read()` / `write()` も、
`ros2_control` 側が持つ制御ループの骨格から呼ばれる各段だと読めます。

## 3.9 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| 派生の実装が呼ばれず、基底の実装が動く | 基底の関数に `virtual` が付いていない。C++ は書かないと仮想にならない |
| 派生の実装が呼ばれない（`virtual` は付けた） | シグネチャがずれて別関数になっている。`override` を付ければエラーになる |
| `error: only virtual member functions can be marked 'override'` | 非仮想の骨格を差し替えようとしている。設計を見直す |
| `error: 'setup' is a private member of 'Sensor'` | `private virtual` を派生から**呼ぼう**としている。呼ぶ必要があるなら `protected` へ |
| `error: declaration of 'setup' overrides a 'final' function` | 基底で `final` が付いている。差し替えは意図的に止められている |
| コンストラクタの中でだけ挙動が違う | 3.4。コンストラクタ／デストラクタ内の仮想呼び出しは基底の実装が動く |
| 実行時に `pure virtual method called` で落ちる | コンストラクタ／デストラクタから**純粋**仮想関数を呼んでいる |
| 基底ポインタで破棄したらリークした | `virtual ~SensorReader()` が無い |
| 派生が骨格を勝手に書き換えている | 骨格に `virtual` を付けてしまっている |
| テストは通るが手順の順番が違う | 骨格の中の呼び出し順。課題のテストはここを見ている |

## 3.10 対応する課題

```bash
./drill run dp03
```

`exercises/dp03_template_method/src/sensor_reader.cpp` に、

1. `SensorReader::read_once()` — テンプレートメソッド（初期化 → 取得 → 変換 → 検証）
2. `SensorReader::validate()` — 既定の検証（フックメソッド）
3. `EncoderReader` — カウント値 → 角度[deg]
4. `ThermistorReader` — AD 値 → 温度[degC]、範囲外を弾く（`validate()` を差し替える）

を実装します。ヘッダ `include/drill/sensor_reader.hpp` は編集しません。
**骨格が各段を呼んだ順序を記録していて、テストはその順序を検査します。**
値が合っていても手順が違えば落ちます。

`ThermistorReader::validate()` だけは基底版を呼ぶ必要があるので、
ヘッダ側で `protected` に置いてあります。**なぜ他の 3 つは `private` なのか**を
3.1 の変更点 3 と照らして確認してください。

## 3.11 この章のまとめ

- Java は**既定で仮想**、C++ は**書かないと仮想でない**。既定が逆
- Java の `final` メソッド ↔ C++ の「`virtual` を書かない」。**骨格には `virtual` を付けない**
- **C++ では `private` な仮想関数もオーバーライドできる。** アクセス指定とオーバーライド可否は独立
- これを使った「public 非仮想の骨格 → private 仮想の各段」が **NVI イディオム**
- 派生が基底版を呼ぶ段だけ `protected virtual`。呼ばないなら `private` のまま
- `override` は例外なく書く。書かないと**シグネチャのズレが黙って別関数になる**
- 差し替えを途中で止めるなら `final`。葉のクラスにも `final`
- **仮想デストラクタ必須**
- **コンストラクタ／デストラクタから仮想関数を呼ばない。** 基底の実装が呼ばれる。Java と逆
- マイコンでも仮想関数版で書ける（確保が起きない）。ただし vptr と vtable のコストは乗る
- 差し替えがコンパイル時に決まるなら **CRTP** で vtable ゼロにできる。実行時の多態を失う

---

前: [2. Adapter](02_Adapter.md) ／ 次: 4. Factory Method（準備中）
