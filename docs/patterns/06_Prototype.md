# 6. Prototype

> **結城本 第6章 対応。** `Product` インタフェースと `Manager`、`UnderlinePen` / `MessageBox` を手元に開いてください。
>
> **この章のねらい**: Java の `clone()` と `Cloneable` は、**Java にコピーの言語機能が無いから**必要だったものです。
> C++ には最初からコピーコンストラクタがあります。だから第6章の答えの半分は「**このパターンは要らない**」です。
> 残りの半分 ―― `std::unique_ptr<Base>` しか持っていないときの複製 ―― だけが本物の Prototype です。
> そこで**共変戻り値型が `unique_ptr` では使えない**という C++ 固有の壁にぶつかります。

## 6.1 まずコピーコンストラクタで足りないか問う

結城本の `Product` はこうです。

```java
public interface Product extends Cloneable {
    public abstract void use(String s);
    public abstract Product createCopy();
}
```

`createCopy()` の中身は `clone()` を呼ぶだけです。
**なぜ Java にこれが要るのか**を先に押さえてください。Java には

```java
UnderlinePen copy = pen;      // 同じオブジェクトを指すだけ。複製ではない
```

しかありません。**変数は全部参照**なので、「中身の複製」を作る言語機能が存在しないのです。
だから `Object.clone()` という protected なメソッドと `Cloneable` マーカーインタフェースで
後付けしています。

C++ は違います。

```cpp
PulseTrain copy = original;   // 複製される。中身がコピーされる
```

**これで終わりです。** コピーコンストラクタは言語機能で、
書かなくてもコンパイラが作ってくれます。

だから第6章を C++ に持ち込むときの最初の判断はこれです。

> **複製したいオブジェクトの型が、その場で分かっているか。**
> 分かっているなら `clone()` は要らない。コピーコンストラクタを使う。

`Manager` に `PulseTrain` を登録して `PulseTrain` として取り出すなら、Prototype は不要です。
`std::map<std::string, PulseTrain>` に入れて、取り出してコピーすれば済みます。

## 6.2 clone() が本当に要るのは多態のときだけ

要るのはこの形のときです。

```cpp
std::unique_ptr<Waveform> original = load_from_config();   // 中身が何かは知らない
std::unique_ptr<Waveform> copy = /* ??? */;
```

`original` の実体が `PulseTrain` なのか `SineSweep` なのか、呼ぶ側は知りません。
コピーコンストラクタは**静的な型に対して選ばれる**ので、ここでは使えません。

```cpp
Waveform copy = *original;    // Waveform は抽象クラス。そもそも書けない
```

必要なのは「**自分が何者かを知っている自分自身に、複製を作らせる**」ことです。
それが仮想関数としての `clone()` です。**Prototype の本質はここだけ**です。

判断はこの表で足ります。

| 状況 | 使うもの |
| --- | --- |
| 型が分かっている | コピーコンストラクタ。`clone()` を書かない |
| `unique_ptr<Base>` しか無い／基底の参照しか無い | 仮想の `clone()` |
| 型の候補が有限で列挙できる | `std::variant`。コピーは既に多態的（6.6） |

## 6.3 共変戻り値型 —— `unique_ptr` では使えない

C++ には**共変戻り値型**（covariant return type）があります。
基底が `Base * f()` なら、派生は `Derived * f()` で override できます。

```cpp
class Waveform
{
public:
  virtual Waveform * clone() const = 0;
};

class SineSweep : public Waveform
{
public:
  SineSweep * clone() const override;      // 通る。Waveform * より狭い型を返せる
};
```

では所有権を型に書くために `std::unique_ptr` にしたらどうなるか。**通りません。**

```cpp
class Waveform
{
public:
  virtual std::unique_ptr<Waveform> clone() const = 0;
};

class SineSweep : public Waveform
{
public:
  std::unique_ptr<SineSweep> clone() const override    // ここ
  {
    return std::make_unique<SineSweep>(*this);
  }
};
```

実際にコンパイルするとこう出ます。

```
error: virtual function 'clone' has a different return type ('unique_ptr<SineSweep>')
       than the function it overrides (which has return type 'unique_ptr<Waveform>')
   13 |   std::unique_ptr<SineSweep> clone() const override
      |   ~~~~~~~~~~~~~~~~~~~~~~~~~~ ^
note: overridden virtual function is here
    7 |   virtual std::unique_ptr<Waveform> clone() const = 0;
      |           ~~~~~~~~~~~~~~~~~~~~~~~~~ ^
```

**共変戻り値型はポインタと参照にしか許されていません。**
`std::unique_ptr<Derived>` は `std::unique_ptr<Base>` の派生クラスではないからです。
テンプレートの実引数が継承関係にあっても、テンプレートの実体化どうしは無関係です。
`std::vector<Derived>` が `std::vector<Base>` の派生でないのと同じ話です。

### 回避策 —— NVI と組み合わせる

「共変にしたい」と「`unique_ptr` を返したい」を両立させるには、**2 段に分けます**。
第3章でやった NVI（public 非仮想 → private 仮想）と同じ形です。

```cpp
class Waveform
{
public:
  virtual ~Waveform() = default;

  // public。非仮想。呼ぶ側にはこれしか見えない
  std::unique_ptr<Waveform> clone() const
  {
    return std::unique_ptr<Waveform>(do_clone());
  }

private:
  // private。仮想。生ポインタなので共変にできる
  virtual Waveform * do_clone() const = 0;
};

class SineSweep : public Waveform
{
private:
  SineSweep * do_clone() const override      // 共変。SineSweep * を返せる
  {
    return new SineSweep(*this);
  }
};
```

得られるもの:

- 呼ぶ側は `std::unique_ptr` を受け取る。**誰が解放するかが型に書かれている**
- 派生側は生ポインタを返す。**共変が効く**ので `SineSweep::do_clone()` を
  `SineSweep *` で受ければキャスト無しで使える
- 裸の `new` が現れるのは `do_clone()` の 1 行だけ。**`clone()` が即座に包む**ので
  例外が飛んでも漏れない

「`unique_ptr` を返す仮想関数を素直に書く」でも動きます（共変を諦めるだけ）。
共変が要らないなら、そちらの方が短くて安全です。**共変が要るのは、
派生の型が分かっている文脈で `clone()` を呼び、キャスト無しで派生の型を受け取りたいとき**です。
この課題では NVI 版で書きます。

## 6.4 誰が所有するのか

結城本の `createCopy()` は `Product` を返すだけです。GC が回収します。
C++ で `Waveform * clone() const;` と書くと、**呼んだ人が `delete` するのかどうかが型に書かれていません**。
Iterator の章と同じ判断で `std::unique_ptr` を返します。

```cpp
std::unique_ptr<Waveform> clone() const;
```

結城本の `Manager` は `HashMap` に原型を溜めます。C++ ではこうです。

```cpp
std::vector<std::unique_ptr<Waveform>> waveforms_;
```

そして「ライブラリを丸ごと複製する」は、要素の実体の型を知らないまま `clone()` を回すだけです。

```cpp
WaveformLibrary WaveformLibrary::duplicate() const
{
  WaveformLibrary copy;
  for (const std::unique_ptr<Waveform> & waveform : waveforms_) {
    copy.waveforms_.push_back(waveform->clone());
  }
  return copy;
}
```

**この関数が Prototype の存在理由そのもの**です。`dynamic_cast` も `if` の連鎖も要りません。

## 6.5 C++ 固有の危険

### 危険1: スライシング

Java では絶対に起きない事故です。

```cpp
SineSweep sweep{10.0, 200.0, 8};
Waveform sliced = sweep;        // SineSweep の部分が切り捨てられる
```

`sliced` は `Waveform` 1 個ぶんの大きさしかありません。**派生の状態も vtable も落ちます。**
6.7 で実際に出力を見ます。

止め方は 2 つです。

| 方法 | 効果 |
| --- | --- |
| 基底に純粋仮想関数を置く（抽象クラスにする） | 値で受けられなくなる。**この課題はこれ** |
| 基底のコピーコンストラクタを `protected` にする | 派生からは呼べるが、外からのスライシングは止まる |

この課題の `Waveform` は両方やっています。実際に `Waveform sliced = pulse;` と書くと、

```
error: variable type 'Waveform' is an abstract class
    5 |   Waveform sliced = pulse;
      |            ^
```

さらに**基底への代入**も同じ事故です。`Waveform & operator=(const Waveform &) = delete;` で止めます。

```cpp
Waveform & a = pulse;
Waveform & b = sweep;
a = b;                          // 中身が混ざる。delete してあれば通らない
```

### 危険2: 浅いコピーと二重解放

Java の `Object.clone()` は既定で**浅いコピー**です。フィールドの参照をそのまま写します。
C++ の暗黙のコピーコンストラクタも同じで、**メンバごとのコピー**です。
生ポインタメンバがあると、こうなります。

```cpp
class PulseTrain
{
public:
  explicit PulseTrain(std::size_t length) : pattern_(new double[length]()), length_(length) {}
  ~PulseTrain() { delete[] pattern_; }

private:
  double * pattern_;
  std::size_t length_;
};

PulseTrain a{4};
PulseTrain b = a;      // pattern_ の「値」がコピーされる = 同じ配列を 2 つが指す
                       // 両方のデストラクタが delete[] する = 二重解放
```

手元で走らせると、**警告ゼロでコンパイルが通り、実行時に SIGABRT で落ちます**（終了コード 133）。
Java では起こりえない壊れ方です。

これを**言語に止めさせる**のが `std::unique_ptr` メンバです。

```cpp
std::unique_ptr<double[]> pattern_;
```

`unique_ptr` はコピーできないので、**この型の暗黙のコピーコンストラクタは自動的に `delete` されます**。
浅いコピーを書こうとするとコンパイルエラーになり、深いコピーが欲しければ自分で書くしかありません。

```cpp
PulseTrain::PulseTrain(const PulseTrain & other)
: Waveform(other),
  label_(other.label_),
  pattern_(new double[other.length_]()),
  length_(other.length_)
{
  std::copy(other.pattern_.get(), other.pattern_.get() + other.length_, pattern_.get());
}
```

基底のコピーコンストラクタ `Waveform(other)` を書き忘れないでください。
書かないと基底部分が**デフォルト構築**されます。警告は出ません。

### 危険3: 「`vector<unique_ptr>` を持てば自動でコピー禁止」は嘘

これは実際に踏みます。

```cpp
class WaveformLibrary
{
private:
  std::vector<std::unique_ptr<Waveform>> waveforms_;
};

static_assert(!std::is_copy_constructible<WaveformLibrary>::value, "…");   // 落ちる
```

`std::vector` はコピーコンストラクタを**宣言だけ**しています。中身がコピーできないことは、
実際にコピーを実体化しようとしたときに初めてエラーになります。
だから `std::is_copy_constructible` は **`true`** を返します。

「コピー禁止」を**型の性質として表明したい**なら、自分で書くしかありません。

```cpp
WaveformLibrary(const WaveformLibrary &) = delete;
WaveformLibrary & operator=(const WaveformLibrary &) = delete;
WaveformLibrary(WaveformLibrary &&) = default;
WaveformLibrary & operator=(WaveformLibrary &&) = default;
```

### Rule of Zero / Rule of Five

上で 4 行書いたのは偶然ではありません。C++ には 5 つの特殊メンバ関数があります。

デストラクタ / コピーコンストラクタ / コピー代入 / ムーブコンストラクタ / ムーブ代入

**Rule of Five**: このうち 1 つでも自分で書いたら、**残り 4 つも意図を書く**
（`= default` / `= delete` / 手書きのいずれか）。
コピーを user-declared にすると**ムーブが暗黙に生成されなくなる**ので、
黙っているとムーブが消えます。

**Rule of Zero**: そもそも 1 つも書かないのが最善。
メンバを `std::string` / `std::vector` / `std::unique_ptr` のような
**自分で後始末する型**だけにすれば、5 つとも正しく自動生成されます。

この課題では:

- `SineSweep` は値メンバだけ → **Rule of Zero**。5 つとも書かない
- `PulseTrain` は `unique_ptr<double[]>` を持ち、深いコピーが要る → コピーコンストラクタを手書きし、
  コピー代入を `= delete`（この課題では要らないので）
- `Waveform` は基底 → 仮想デストラクタ、コピーコンストラクタ `protected`、コピー代入 `= delete`

## 6.6 標準ライブラリ／言語機能に同じものが無いか

**コピーコンストラクタそのものが Prototype です。** 言語機能として最初から入っています。
6.1 で見たとおり、Java が `clone()` を必要としたのは、この機能が無かったからです。

多態的な複製に限れば、標準ライブラリに既製品はありません。`clone()` は自分で書きます。
ただし**多態を避ける道**なら標準にあります。

```cpp
using Waveform = std::variant<PulseTrain, SineSweep>;

Waveform original = SineSweep{10.0, 200.0, 8};
Waveform copy = original;        // これで正しく複製される。clone() は不要
```

`std::variant` は「型の候補が有限で、コンパイル時に列挙できる」ときに使えます。
**そのときコピーは既に多態的**です。`clone()` も vtable もヒープ確保も要りません。
波形の種類がコンパイル時に決まっているなら、これが一番短い答えです。
（`std::variant` と `std::visit` は第13章 Visitor で本格的に扱います。）

`std::shared_ptr` は複製ではなく**共有**です。混同しないでください。

```cpp
std::shared_ptr<Waveform> b = a;   // 複製ではない。同じオブジェクトを 2 人で持つだけ
```

Java の `=` に一番近いのはこちらです。「Java のつもり」で書くとこうなるので、
**複製したいのか共有したいのか**を毎回はっきりさせてください。

## 6.7 手元で試す

スライシングと `clone()` を並べます。**出力を予想してから**実行してください。

```cpp
#include <iostream>
#include <memory>
#include <string>

class Waveform
{
public:
  virtual ~Waveform() = default;
  virtual std::string name() const { return "Waveform"; }

  // public 非仮想の clone。中身は private 仮想の do_clone に任せる（NVI）
  std::unique_ptr<Waveform> clone() const { return std::unique_ptr<Waveform>(do_clone()); }

private:
  virtual Waveform * do_clone() const { return new Waveform(*this); }
};

class SineSweep : public Waveform
{
public:
  std::string name() const override { return "SineSweep"; }

private:
  // 生ポインタなら共変戻り値型が使える
  SineSweep * do_clone() const override { return new SineSweep(*this); }
};

int main()
{
  SineSweep sweep;

  // (1) 値で受ける = スライシング
  Waveform sliced = sweep;
  std::cout << "sliced: " << sliced.name() << "\n";

  // (2) clone() 経由 = 派生の型のまま複製される
  std::unique_ptr<Waveform> original = std::make_unique<SineSweep>();
  std::unique_ptr<Waveform> copy = original->clone();
  std::cout << "clone : " << copy->name() << "\n";
  std::cout << "same address? " << (original.get() == copy.get() ? "yes" : "no") << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: <code>sliced.name()</code> は何を返すか。警告は出るか</summary>

```
sliced: Waveform
clone : SineSweep
same address? no
```

`sliced` は `SineSweep` から作ったのに `"Waveform"` を返します。
**派生部分が丸ごと切り捨てられています。** そして
`-Wall -Wextra -Wpedantic` で**警告は 1 つも出ません**。Java なら起こりえない事故が、無言で通ります。

`do_clone()` を `std::unique_ptr<SineSweep>` に変えて `clone()` を直接仮想にすると、
6.3 のコンパイルエラーが再現できます。試してください。

`Waveform` に純粋仮想関数を 1 つ足すと `Waveform sliced = sweep;` の行が
コンパイルエラーになります。**抽象クラスはスライシングを型で止めます。**
</details>

## 6.8 マイコンでの結論

`clone()` は**素直に書くとヒープを使います**。ループの中で呼べば断片化します。
さらに `-fno-rtti` だと `dynamic_cast` が使えないので、
「複製したあとで型を確かめる」という書き方も封じられます。

順に選んでください。

**第一候補: そもそも Prototype を使わない。値でコピーする。**

```cpp
// 制御パラメータのプリセット。多態は要らない
struct GainPreset
{
  float kp;
  float ki;
  float kd;
};

// ROM 上のひな型
constexpr GainPreset kArmPreset{2.0F, 0.1F, 0.05F};

void start_arm_control()
{
  GainPreset preset = kArmPreset;     // これが複製。確保ゼロ
  preset.kp *= 0.8F;                  // 現場合わせ
  apply(preset);
}
```

`constexpr` なひな型は **ROM に置かれ、RAM を 1 バイトも食いません**。
「原型を登録しておいて複製して使う」という Prototype の目的が、これで満たされています。

**第二候補: 多態が要るなら、固定プールに構築する。**

```cpp
// ヒープを使わず、あらかじめ確保した領域に複製を作る
class Waveform
{
public:
  virtual ~Waveform() = default;

  /// buffer に自分の複製を構築して、その先頭を返す。
  /// 容量が足りなければ nullptr（-fno-exceptions なので throw しない）。
  virtual Waveform * clone_into(void * buffer, std::size_t capacity) const = 0;

  virtual std::size_t object_size() const = 0;
};

class SineSweep : public Waveform
{
public:
  Waveform * clone_into(void * buffer, std::size_t capacity) const override
  {
    if (capacity < sizeof(SineSweep)) {
      return nullptr;
    }
    return new (buffer) SineSweep(*this);   // placement new。確保はしない
  }

  std::size_t object_size() const override { return sizeof(SineSweep); }
};
```

placement new は**確保しません**。呼ぶ側が用意した領域に構築するだけです。
その代わり **`delete` してはいけません**。デストラクタを明示的に呼びます。

```cpp
alignas(SineSweep) unsigned char pool[64];

Waveform * copy = prototype.clone_into(pool, sizeof(pool));
if (copy != nullptr) {
  // 使う
  copy->~Waveform();      // delete ではなくデストラクタ直接呼び出し
}
```

`alignas` を忘れると、アラインメント違反で落ちる MCU があります（Cortex-M0 など）。

vtable のコストも忘れないでください。`clone_into` と `object_size` と仮想デストラクタで、
派生 1 つにつき vtable が ROM に載ります。**波形の種類が 2 つなら、
`enum` と `switch` の方が小さくて速い**ことが普通です。

## 6.9 ROS 2 での結論（補足）

rclcpp に GoF 版の `Prototype` は出てきません。メッセージ型は
`sensor_msgs::msg::Imu` のような**素の構造体**で、コピーコンストラクタがそのまま使えます。

```cpp
sensor_msgs::msg::Imu imu_copy = imu_msg;   // これで複製
```

注意すべきは `publish()` の側です。`std::unique_ptr` で渡すと**ムーブ**（所有権の移動）で、
const 参照で渡すと middleware 側でコピーされます。**複製と共有と移動を混同しない**こと。
これは第14章の zero-copy の話につながります。

`rclcpp::Parameter` や `rclcpp::QoS` も値型で、コピーで複製できます。
`clone()` を探す場面はまずありません。

## 6.10 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: virtual function 'clone' has a different return type` | `unique_ptr` で共変戻り値型を使おうとしている。6.3 の NVI 版にする |
| 複製したのに元を変えると複製も変わる | 浅いコピー。ポインタメンバの中身を写していない |
| 実行時に SIGABRT（終了コード 133） | 生ポインタメンバの二重解放。`unique_ptr` にする |
| `clone()` したのに `name()` が基底のものを返す | `do_clone()` が基底を `new` している。`new Derived(*this)` にする |
| 値で受けたら派生の情報が消えた | スライシング。基底を抽象クラスにするかコピーを `protected` に |
| コピーコンストラクタを書いたらムーブが効かなくなった | Rule of Five。ムーブも `= default` で明示する |
| 基底のメンバが複製されない | 派生のコピーコンストラクタで `Base(other)` を書いていない |
| `static_assert(!is_copy_constructible<...>)` が落ちる | `vector<unique_ptr>` を持っても暗黙のコピーは delete されない。6.5 の危険3 |

## 6.11 対応する課題

```bash
./drill run dp06
```

`exercises/dp06_prototype/src/waveform.cpp` に、

1. `Waveform::clone()` — `do_clone()` を `std::unique_ptr` に包む
2. `PulseTrain` のコピーコンストラクタ — 深いコピー
3. `PulseTrain::do_clone()` / `SineSweep::do_clone()` — 共変戻り値型
4. `WaveformLibrary::duplicate()` — 実体の型を知らないまま全要素を複製

を実装します。テストは**深いコピーになっているか**（元を書き換えて確かめる）、
**`unique_ptr<Waveform>` 経由でも派生の型が保たれるか**（`dynamic_cast`）、
**複製が別のオブジェクトか**（アドレス比較）を見ます。
`static_assert` で、コピーが禁止されているべきクラスが実際に禁止されているかも検査します。

## 6.12 この章のまとめ

- Java の `clone()` は **Java にコピーの言語機能が無いから**あった。C++ にはある
- **まずコピーコンストラクタで足りないか問う。** 型が分かっているなら `clone()` は要らない
- `clone()` が要るのは **`unique_ptr<Base>` しか無いとき**だけ
- **共変戻り値型はポインタと参照だけ。** `unique_ptr<Base>` → `unique_ptr<Derived>` は不可
- 回避策は **NVI**。public 非仮想 `clone()` が、private 仮想 `do_clone()`（生ポインタ・共変）を包む
- **スライシング**は Java に無い事故。抽象クラスか `protected` なコピーコンストラクタで止める
- 生ポインタメンバの暗黙コピーは**二重解放**。`unique_ptr` メンバにすれば言語が止める
- **Rule of Zero が第一。** 1 つ書いたら Rule of Five で 5 つとも意図を書く
- 型の候補が有限なら **`std::variant`**。コピーが既に多態的になる
- マイコンでは第一に「使わない・値でコピー」、必要なら **placement new で固定プールに複製**

---

前: [5. Singleton](05_Singleton.md) ／ 次: 7. Builder（準備中）
