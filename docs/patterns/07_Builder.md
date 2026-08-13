# 7. Builder

> **結城本 第7章 対応。** `Builder` / `Director` / `TextBuilder` / `HTMLBuilder` を手元に開いてください。
>
> **この章のねらい**: 「Builder」と呼ばれているものは、実は**2 つあります**。
> 結城本のものは「手順（Director）と部品（Builder）を分ける」パターン。
> 実務で見る `.set_a().set_b().build()` は「**C++ に名前付き引数が無い**」を埋めるための道具です。
> 同じ名前ですが解いている問題が違います。この章では両方書きますが、**混ぜません**。
> C++ 固有の話は、チェーンの戻り値（参照か値か）、**参照修飾子** `build() &&`、
> そして `constexpr` で**コンパイル時に組み立てて ROM に置く**ところです。

## 7.1 まず、名前が同じだけの別物を切り離す

先にこれをやらないと、この章はずっと混乱したままになります。

| | 結城本の Builder（GoF） | 実務の Builder（メソッドチェーン） |
| --- | --- | --- |
| 解いている問題 | **同じ手順**から**違う表現**を作りたい | 引数が多すぎて呼び出しが読めない |
| 登場人物 | Director / Builder / ConcreteBuilder | Builder 1 つだけ |
| 手順を持つのは | **Director** | **呼び出し側**（好きな順に呼べる） |
| 差し替えるもの | ConcreteBuilder（多態） | 何も差し替えない。仮想関数すら出てこない |
| 典型例 | 同じデータを CSV でも JSON でも出す | `MotorConfig` を 7 個の引数なしで作る |

GoF の本にも「メソッドチェーン」は一言も出てきません。
`.set_a().set_b().build()` は、**Java の Effective Java 項目2 が広めた別の技法**です。
どちらも有用ですが、**「Builder パターンを使いました」と言われたらどちらか聞いてください。**

この講習では、記事も課題も両方扱います。7.2〜7.3 が結城本の形、7.4 以降が実務の形です。

## 7.2 Java 版をそのまま C++ にすると

結城本の `Builder` はこうです（抽象クラス）。

```java
public abstract class Builder {
    public abstract void makeTitle(String title);
    public abstract void makeString(String str);
    public abstract void makeItems(String[] items);
    public abstract void close();
}
```

C++ に素直に移すとこうなります。

```cpp
class TelemetryBuilder
{
public:
  virtual ~TelemetryBuilder() = default;
  virtual void make_header(const std::string & title) = 0;
  virtual void make_field(const std::string & key, double value) = 0;
  virtual void make_footer() = 0;
};
```

変更点が 3 つあります。

### 変更点1: `virtual ~TelemetryBuilder() = default;` を足した

第1章から毎回同じです。**純粋仮想関数を 1 つでも書いたら仮想デストラクタ**。
この章の `Director` は Builder を参照でしか持たないので今すぐ落ちはしませんが、
将来 `std::unique_ptr<TelemetryBuilder>` で持った瞬間に未定義動作になります。
**あとから足す**より最初から書きます。

### 変更点2: `String title` を `const std::string & title` にした

Java の `String` は参照渡しです。C++ で `void make_header(std::string title)` と書くと
**呼ぶたびに文字列をコピー**します。読むだけなら `const std::string &` です。

`String[] items` に至っては、C++ で `std::vector<std::string>` を値で受けると
配列ごとコピーになります。この課題では `make_field(key, value)` を複数回呼ぶ形にしました。
**「配列を渡す」を「1 個ずつ渡す」に変えるだけでコピーが消える**ことは覚えておいてください。

### 変更点3: `getResult()` を基底クラスに置かなかった

結城本でも `getResult()` は `TextBuilder` / `HTMLBuilder` 側にあります。
理由は Java でも C++ でも同じで、**戻り型が ConcreteBuilder ごとに違いうる**からです。

```cpp
class CsvTelemetryBuilder : public TelemetryBuilder
{
public:
  // ...
  const std::string & result() const { return text_; }   // 基底には置けない
};
```

C++ ではさらに一歩踏み込めます。`result()` は**仮想である必要がありません**。
`Director` は `result()` を呼ばないからです。呼ぶのは、
`CsvTelemetryBuilder` を具体型として知っている `main` 側だけです。
**仮想関数は「多態で呼ばれるものだけ」に絞ります。** vtable が小さくなります。

なお `const std::string &` を返しています。`std::string` を返すとコピーが 1 回走ります。
「Builder はまだ生きている」場面で使うので、参照で足ります。

## 7.3 誰が所有するのか

結城本の `main` はこう書きます。

```java
Builder builder = new TextBuilder();
Director director = new Director(builder);
director.construct();
```

C++ ではこうです。**`new` は要りません。**

```cpp
CsvTelemetryBuilder builder;              // スタック上
TelemetryDirector director{builder};      // 参照で持つだけ
director.construct();
std::cout << builder.result();
```

第1章・第4章では「Java の `new` は `std::unique_ptr`」と言いましたが、
**ここは違います。** `Builder` の寿命は `main` のスコープと同じで、誰にも渡りません。
所有権が動かないなら、**動的確保はしません**。

その代わり `Director` が `TelemetryBuilder &` を参照で持つので、**寿命の約束**が要ります。

```cpp
class TelemetryDirector
{
public:
  explicit TelemetryDirector(TelemetryBuilder & builder) : builder_(builder) {}
  void construct();

private:
  TelemetryBuilder & builder_;   // Director は Builder より長生きできない
};
```

Java なら GC が Builder を生かします。C++ では、
Builder が先に死んだ `Director` を呼ぶと未定義動作です。第1章 1.3 と同じ話です。

`explicit` を付けているのも C++ 固有です。付けないと
`TelemetryDirector director = builder;` が通ってしまいます（暗黙変換）。
**引数 1 個のコンストラクタには `explicit`。** Java にこの問題はありません。

## 7.4 ここから実務の形 — C++ に名前付き引数が無い

モータの設定を作ります。項目は 7 つです。

```cpp
struct MotorConfig
{
  std::uint8_t motor_id = 0;
  std::string name = "unnamed";
  double max_duty = 1.0;
  double current_limit_ampere = 5.0;
  std::uint32_t encoder_counts_per_rev = 4096;
  bool invert_direction = false;
  bool brake_on_stop = true;
};
```

コンストラクタで作るとこうなります。

```cpp
MotorConfig config{3, "drive_left", 0.8, 12.0, 8192, true, false};
```

**最後の `true, false` が何なのか、この行からは読めません。**
`invert_direction` と `brake_on_stop` を逆に書いても**コンパイルは通ります**。
そして実機で、止まらないモータが逆回転します。

Python なら `MotorConfig(motor_id=3, invert_direction=True)` と書けます。
C++ には名前付き引数がありません。**これが実務の Builder の動機です。**
パターンが好きだからではありません。

### 対案1: 強い型（strong typedef）を作る

`bool` を全部やめて、意味を持つ型にします。

```cpp
enum class Direction : std::uint8_t { normal, inverted };
enum class StopBehavior : std::uint8_t { coast, brake };

MotorConfig config{3, "drive_left", 0.8, 12.0, 8192,
                   Direction::inverted, StopBehavior::coast};
```

**これは Builder より優れています。** 引数の順番を間違えたら**コンパイルエラー**になるからです。
Builder は実行時にしか間違いを見つけません。
`bool` の羅列を見たら、まず `enum class` にできないか考えてください。

ただし、`double max_duty` と `double current_limit_ampere` のような
**同じ型の数値が並ぶ**ところは、これでは救えません。そこが Builder の出番です。

### 対案2: 設定用の構造体を渡す

```cpp
MotorConfig config;
config.motor_id = 3;
config.name = "drive_left";
config.max_duty = 0.8;
```

読めます。**そして Builder より単純です。**
欠点は 2 つで、**`const` にできない**ことと、
**「必須項目が埋まっているか」を誰もチェックしない**ことです。
`motor_id` を書き忘れても 0 番モータの設定として動いてしまいます。

C++20 なら designated initializer で `MotorConfig config{.motor_id = 3, .max_duty = 0.8};`
と書けて、`const` も付けられます。**標準では C++20 からです。**
（手元の Apple clang は `-std=c++17 -pedantic-errors` でもこれを通しました。
コンパイラ拡張として受け入れられているだけで、規格上は C++17 の機能ではありません。
別のコンパイラや古いツールチェーンでは通りません。C++17 のコードベースでは頼れません。）

**判断はこうです。**

| 状況 | 選ぶもの |
| --- | --- |
| `bool` が並んでいる | `enum class`（対案1） |
| 項目が 3〜4 個で全部任意 | 設定構造体（対案2） |
| 必須項目とオプション項目が混ざる | **Builder** |
| コンパイル時に値が決まる | **`constexpr` Builder**（7.11） |

## 7.5 チェーンの戻り値 — 参照を返す

```cpp
class MotorConfigBuilder
{
public:
  MotorConfigBuilder & motor_id(std::uint8_t id);
  MotorConfigBuilder & name(std::string motor_name);
  // ...
};
```

**`&` です。** ここを値にするとこうなります。

```cpp
MotorConfigBuilder motor_id(std::uint8_t id);   // 悪い
```

`.motor_id(3).name("drive_left").max_duty(0.8)` と 3 回つなぐと、
**Builder 全体のコピーが 3 回**走ります。`MotorConfig` は `std::string` を持つので、
コピーのたびにヒープ確保が起きます。Java では参照が返るのが当たり前なので、
Java 版の Builder を写すとここを間違えます。

課題のテストはこれをアドレスで見ます。

```cpp
MotorConfigBuilder builder;
EXPECT_EQ(&builder, &builder.motor_id(7));   // 値で返していると落ちる
```

セッタの引数も同じ話です。`name` は `std::string` を**値で受けて `std::move`** します。

```cpp
MotorConfigBuilder & MotorConfigBuilder::name(std::string motor_name)
{
  config_.name = std::move(motor_name);
  return *this;
}
```

`const std::string &` で受けて代入すると、呼び出し側が一時オブジェクトを渡したときも
必ずコピーが 1 回走ります。値で受けて `move` すれば、一時オブジェクトなら確保ゼロです。

## 7.6 参照修飾子 — `build() &&` と `build() const &`

ここが**この章でいちばん C++ らしい**ところです。Java には存在しません。

`build()` は 2 通りの呼ばれ方をします。

```cpp
// (A) 使い捨て。Builder はこの行で消える
const auto config = MotorConfigBuilder{}.motor_id(3).name("drive_left").build();

// (B) 名前を付けて、条件で分岐しながら埋める。Builder はこのあとも生きている
MotorConfigBuilder builder;
builder.motor_id(3);
if (is_left_side) { builder.invert_direction(true); }
const auto config = builder.build();
```

(A) では Builder の中身を**持っていって構いません**。もう誰も見ないからです。
(B) では**持っていってはいけません**。Builder はまだ使われるかもしれません。

この 2 つは、メンバ関数に `&` / `&&` を付けると**分けて書けます**。

```cpp
class MotorConfigBuilder
{
public:
  std::optional<MotorConfig> build() const &;   // 左辺値から呼ばれた → コピー
  std::optional<MotorConfig> build() &&;        // 右辺値から呼ばれた → ムーブ
};
```

```cpp
std::optional<MotorConfig> MotorConfigBuilder::build() const &
{
  if (!has_motor_id_) { return std::nullopt; }
  return config_;                 // コピー
}

std::optional<MotorConfig> MotorConfigBuilder::build() &&
{
  if (!has_motor_id_) { return std::nullopt; }
  return std::move(config_);      // ムーブ。std::string の確保が 1 回減る
}
```

**呼び分けはコンパイラが自動でやります。** 呼び出し側は何も書きません。
`MotorConfigBuilder{}...build()` は `&&` 版、`builder.build()` は `const &` 版です。
明示的に `&&` 版を呼びたいときだけ `std::move(builder).build()` と書きます。

### 片方しか書かないとどうなるか

`build() &&` だけを書いて、左辺値から呼ぶとこうなります（実際の出力）。

```
error: 'this' argument to member function 'build' is an lvalue, but function has rvalue ref-qualifier
   17 |   Config config = builder.build();
      |                   ^
note: 'build' declared here
    7 |   Config build() && { return config_; }
      |          ^
```

**参照修飾子は片方だけ書くと、もう片方が使えなくなります。**
「使い捨てでしか使わせない」と決めたなら `&&` だけにするのは有効な設計です
（`.build()` を 2 回呼ぶバグを**コンパイル時に**防げます）。
そうでないなら**両方書きます**。片方だけ書いて後悔するのがよくある事故です。

## 7.7 必須項目が埋まっていないとき

`motor_id` に安全な既定値はありません。0 番は実在するモータです。
埋まっていないまま `build()` されたらどうするか。**3 択です。**

| 方法 | 書き方 | 向き不向き |
| --- | --- | --- |
| 例外を投げる | `throw std::logic_error{...}` | Linux / ROS 2 ならこれでよい。**マイコンでは使えない** |
| `std::optional` を返す | `return std::nullopt;` | どこでも動く。**呼び出し側が確認を忘れうる** |
| 型で強制する | 必須項目をコンストラクタ引数にする | 一番強い。**実行時チェックが要らなくなる** |

マイコンは `-fno-exceptions` が普通なので、`throw` は書けません。
**この課題は `std::optional` にしました。** マイコンと ROS 2 で同じコードが動くからです。

3 番目は書いておく価値があります。

```cpp
class MotorConfigBuilder
{
public:
  // 必須項目はここでしか渡せない。埋め忘れは「書けない」
  explicit MotorConfigBuilder(std::uint8_t motor_id) { config_.motor_id = motor_id; }

  MotorConfig build() const &;   // optional が要らなくなる
};
```

**必須項目が 1〜2 個ならこれが最善です。** 実行時チェックが消え、`optional` も消えます。
必須が 5 個あるなら Builder の意味が薄いので、そのときは設計を疑ってください。

（「型で状態を進める」やり方 — 必須項目を埋めるたびに別の型を返して、
全部埋まった型にだけ `build()` を生やす — もあります。完全にコンパイル時に防げますが、
型が項目数だけ増えます。部活のライブラリには重すぎます。）

## 7.8 不変オブジェクトを作る道具として

Builder の本当の効き目はここです。

```cpp
const MotorConfig config = *MotorConfigBuilder{}.motor_id(3).max_duty(0.8).build();
```

**`const` が付いています。** 組み立てのあいだは Builder が可変で、
出来上がった `MotorConfig` は最初から最後まで書き換えられません。
設定構造体を順に埋めるやり方（7.4 対案2）では、`config` を `const` にできません。

不変にすると何が変わるか。

- 「どこかで誰かが `max_duty` を書き換えた」が**起こりえなくなる**
- スレッドから同時に読んでも安全（ROS 2 のマルチスレッド実行）
- 割り込みハンドラから読んでも、値が途中で変わらない（マイコン）

**「可変な Builder」と「不変な成果物」の組み合わせ**が Builder の本体です。
チェーンで書けるのはおまけです。

## 7.9 標準ライブラリに同じものが無いか

**ありません。** GoF の Builder に相当するものは標準ライブラリにありません。
第1章の Iterator、第21章の Proxy（スマートポインタ）とは事情が違います。

近いものを挙げるなら、`std::ostringstream` です。

```cpp
std::ostringstream oss;
oss << "battery " << 12.5 << " V";
const std::string text = oss.str();
```

「少しずつ足していって、最後に取り出す」形は同じです。
`operator<<` が `std::ostream &` を返すからチェーンできる、という仕組みも 7.5 と同じです。
**`std::ostringstream` は文字列に特化した Builder** だと思って読むと納得できます。

C++20 まで行くと `std::format` があり、文字列組み立ての多くはそちらで済みます。
`std::optional`（7.7）と参照修飾子（7.6）は C++17 で使えます。

## 7.10 手元で試す

課題を解く前に、この 1 ファイルをコンパイルして**出力を予想してから**実行してください。

```cpp
#include <iostream>
#include <string>
#include <utility>

struct Config
{
  std::string name = "unnamed";
  int retry = 0;
};

class Builder
{
public:
  Builder & name(std::string value) { config_.name = std::move(value); return *this; }
  Builder & retry(int value) { config_.retry = value; return *this; }

  Config build() const & { std::cout << "build() const &\n"; return config_; }
  Config build() && { std::cout << "build() &&\n"; return std::move(config_); }

  const Config & peek() const { return config_; }

private:
  Config config_{};
};

struct Limits
{
  float max_velocity = 10.0F;
  float max_current = 5.0F;
};

class LimitsBuilder
{
public:
  constexpr LimitsBuilder & max_velocity(float v) { limits_.max_velocity = v; return *this; }
  constexpr LimitsBuilder & max_current(float v) { limits_.max_current = v; return *this; }
  constexpr Limits build() const { return limits_; }

private:
  Limits limits_{};
};

// コンパイル時に組み立てが終わる。実行時には何も起きない
constexpr Limits kDrive = LimitsBuilder{}.max_velocity(20.0F).build();
static_assert(kDrive.max_velocity == 20.0F, "");
static_assert(kDrive.max_current == 5.0F, "");

int main()
{
  const std::string long_name(64, 'x');   // SSO に収まらない長さにする

  Builder builder;
  std::cout << "same object? "
            << (&builder == &builder.name(long_name).retry(3)) << "\n";

  const char * before = builder.peek().name.data();

  const Config copied = builder.build();
  std::cout << "copied buffer moved? " << (copied.name.data() == before) << "\n";

  const Config moved = std::move(builder).build();
  std::cout << "moved buffer moved?  " << (moved.name.data() == before) << "\n";

  std::cout << "constexpr: " << kDrive.max_velocity << " " << kDrive.max_current << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: どちらの <code>build()</code> がいつ呼ばれ、文字列のバッファはどう動くか</summary>

```
same object? 1
build() const &
copied buffer moved? 0
build() &&
moved buffer moved?  1
constexpr: 20 5
```

読みかたは 4 つあります。

1. `same object? 1` — セッタが `Builder &` を返しているので、チェーンの途中でコピーは起きていません
2. `builder.build()` は**名前の付いた変数から**呼んでいるので `const &` 版。
   `copied buffer moved? 0` はバッファが別物、つまり**コピーされた**ということです
3. `std::move(builder).build()` は `&&` 版。`1` なので、
   **64 文字ぶんのバッファがそのまま持っていかれた**ことが分かります。確保ゼロです
4. `constexpr` の 2 つの `static_assert` が通っています。
   `kDrive` は実行時に一度も組み立てられていません。**コンパイル時に完成しています**

`long_name` を `std::string long_name = "abc";` に変えると 3 が `0` になることがあります。
短い文字列は `std::string` の中に直接入る（SSO）ので、ムーブでもバッファは移らないからです。
**「ムーブは速い」は、ヒープを持つ大きさになってから**の話です。
</details>

## 7.11 マイコンでの結論

**結城本の形（Director + Builder）は、そのままでは重いです。**

- `ConcreteBuilder` が仮想関数 3 つ＋仮想デストラクタ → vtable が ROM に載る
- `std::string` で組み立てる → ループのたびにヒープ確保

テレメトリを CSV でも JSON でも出したい、が実機で本当に要るのかをまず疑ってください。
出力形式が 1 つなら、Director も Builder も要りません（[0. 使う前に](00_使う前に.md) 0.1）。

**実務の形は、`constexpr` にすると化けます。** これがマイコンでの本命です。

```cpp
struct ControlLimits
{
  float max_velocity_rad_per_sec = 10.0F;
  float max_accel_rad_per_sec2 = 50.0F;
  float max_current_ampere = 5.0F;
};

class ControlLimitsBuilder
{
public:
  constexpr ControlLimitsBuilder & max_velocity(float value)
  {
    limits_.max_velocity_rad_per_sec = value;
    return *this;
  }
  constexpr ControlLimitsBuilder & max_accel(float value)
  {
    limits_.max_accel_rad_per_sec2 = value;
    return *this;
  }
  constexpr ControlLimitsBuilder & max_current(float value)
  {
    limits_.max_current_ampere = value;
    return *this;
  }
  constexpr ControlLimits build() const { return limits_; }

private:
  ControlLimits limits_{};
};

// ここで組み立てが終わる。実行時には 1 命令も走らない
constexpr ControlLimits kDriveLimits =
  ControlLimitsBuilder{}.max_velocity(20.0F).max_accel(80.0F).build();

static_assert(kDriveLimits.max_velocity_rad_per_sec == 20.0F, "");
static_assert(kDriveLimits.max_current_ampere == 5.0F, "");   // 既定値のまま
```

得られるもの:

- **`kDriveLimits` は `.rodata`（ROM）に置かれます。** RAM を 1 バイトも使いません
- 組み立てのコードは生成されません。`ControlLimitsBuilder` は実行時には存在しません
- 値の妥当性を `static_assert` で**コンパイル時に**検査できます
- それでいて呼び出しは名前付きで読めます

C++14 以降、`constexpr` 関数の中でメンバを書き換えられるので、
**チェーンで埋めていく書き方がそのままコンパイル時に走ります**。C++11 では書けませんでした。

制約は 2 つです。

1. **`std::string` を入れた瞬間に `constexpr` にできません。** 固定長配列か `string_view` にします
2. **仮想関数を入れると `constexpr` にできません**（C++17 では）。
   つまり結城本の Director + Builder と `constexpr` は同居しません。**どちらかです**

例外が使えないので、必須項目のチェックは `std::optional`（7.7）か、
コンストラクタ引数で必須にする方法を使います。`throw` は書けません。

## 7.12 ROS 2 での結論（補足）

rclcpp では `rclcpp::QoS` がまさにこの形です。

```cpp
auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().transient_local();
```

必須項目（履歴の深さ）は**コンストラクタ引数**、任意項目はチェーンです。
7.7 の 3 番目「必須はコンストラクタで強制する」を採用した実例です。

`rclcpp::NodeOptions` も同じで、`.use_intra_process_comms(true).automatically_declare_parameters_from_overrides(true)`
とつなげます。どちらも**結城本の Director は出てきません**。実務の形だけです。

パラメータ宣言まわりの `rcl_interfaces::msg::ParameterDescriptor` は
7.4 対案2 の「設定構造体を直接埋める」形で、Builder はありません。
**rclcpp の中でも一貫していない**ので、真似するときは「なぜその形か」を見てください。

## 7.13 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| チェーンするほど遅い。プロファイラで `std::string` のコピーが出る | セッタが `Builder`（値）を返している。`Builder &` にする |
| `error: 'this' argument to member function 'build' is an lvalue, but function has rvalue ref-qualifier` | `build() &&` しか書いていない。`build() const &` も書く |
| `build()` を 2 回呼んだら 2 回目が空になった | `build() &` 版で `std::move` している。左辺値版はコピー |
| `std::move(builder).build()` にしてもコピーが減らない | 文字列が短くて SSO に収まっている。ムーブの効果はヒープを持つ大きさから |
| `constexpr` を付けたらコンパイルが通らない | メンバに `std::string` か仮想関数がある。`constexpr` と同居しません |
| `TelemetryDirector director = builder;` が通ってしまう | コンストラクタに `explicit` が無い |
| Director を返す関数から返したら落ちた | Director が Builder を参照で持っている。Builder が先に死んでいる |
| 引数の順番を間違えたのに気づかなかった | `bool` の羅列。Builder より先に `enum class`（7.4 対案1） |
| `build()` の戻りを確認せずに `*` した | `std::optional` を返している。`has_value()` を見る |

## 7.14 対応する課題

```bash
./drill run dp07
```

**結城本の形** — `exercises/dp07_builder/src/telemetry_builder.cpp`

1. `TelemetryDirector::construct()` — 決められた順で Builder を 5 回呼ぶ。
   **書式を一切書かない**のが仕事
2. `CsvTelemetryBuilder` — CSV で組み立てる
3. `JsonTelemetryBuilder` — JSON で組み立てる。カンマを項目の**前**に置くのがコツ

**実務の形** — `exercises/dp07_builder/src/motor_config.cpp`

4. `MotorConfigBuilder` のセッタ 7 つ — **`MotorConfigBuilder &` を返す**
5. `MotorConfigBuilder::build() const &` — コピーして返す。必須が欠けたら `std::nullopt`
6. `MotorConfigBuilder::build() &&` — `std::move` して返す

テストが見るもの:

- 同じ `Director` から CSV と JSON の両方が出ること
- Director の**呼び出し順序**（記録用の Builder を差し込んで確かめます）
- チェーンで設定した値がすべて反映され、設定していない項目は既定値になること
- 必須項目が欠けた `build()` が `std::nullopt` を返すこと
- チェーンが**同じ Builder のアドレス**を返すこと（コピーが起きていないこと）
- `build() &&` がバッファをムーブし、`build() const &` がコピーすること
- `constexpr` Builder が**コンパイル時に**組み上がること（`static_assert`）

## 7.15 この章のまとめ

- 「Builder」は**2 つある**。結城本の Director + Builder と、実務のメソッドチェーン。**混ぜない**
- 結城本の形では `Director` が手順を、`ConcreteBuilder` が書式を持つ。
  `result()` は**基底に置かない**し、**仮想にもしない**
- Builder の寿命はスコープで足りる。**`unique_ptr` は要らない**。代わりに参照の寿命に責任を持つ
- 実務の Builder の動機は「**C++ に名前付き引数が無い**」こと。
  先に `enum class`（強い型）と設定構造体を検討する
- セッタは **`Builder &` を返す**。値で返すとチェーンのたびにコピー
- **`build() const &` はコピー、`build() &&` はムーブ**。参照修飾子で分ける。Java には無い
- 必須項目は `optional` か**コンストラクタ引数で強制**。マイコンでは `throw` できない
- Builder の本体は「可変な Builder → **不変な成果物**」。`const` が付けられることが価値
- 標準ライブラリに Builder は**無い**。近いのは `std::ostringstream`
- マイコンでは **`constexpr` Builder**。ROM に置けて RAM ゼロ、実行時コストゼロ。
  ただし `std::string` と仮想関数とは同居しない

---

前: [6. Prototype](06_Prototype.md) ／ 次: 8. Abstract Factory（準備中）
