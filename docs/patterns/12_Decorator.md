# 12. Decorator

> **結城本 第12章 対応。** `Display` / `Border` / `SideBorder` / `FullBorder` を手元に開いてください。
>
> **この章のねらい**: Java 版の `Border` は `Display display;` というフィールドを 1 つ持つだけです。
> C++ では、この 1 行に **「中身を所有するのか、借りているだけなのか」** という判断が要ります。
> 所有するなら `std::unique_ptr<Display>`、借りるなら `Display &`。
> 前者を選ぶと、組み立てが**ムーブの連鎖**になり、書き方が Java とかなり変わります。
> そして仮想デストラクタを忘れると、**入れ子の内側が丸ごと解放されません**。

## 12.1 Java 版をそのまま C++ にすると

結城本の `Border` はこうです。

```java
public abstract class Border extends Display {
    protected Display display;
    protected Border(Display display) {
        this.display = display;
    }
}
```

C++ に素直に移すとこうなります。

```cpp
class LogSink
{
public:
  virtual ~LogSink() = default;
  virtual std::string format(const std::string & message) const = 0;
};

class SinkDecorator : public LogSink
{
public:
  explicit SinkDecorator(std::unique_ptr<LogSink> inner)
  : inner_(std::move(inner))
  {
  }

protected:
  const LogSink & inner() const { return *inner_; }

private:
  std::unique_ptr<LogSink> inner_;
};
```

Java 版から変えた点が 3 つあります。

### 変更点1: `Display display;` が `std::unique_ptr<LogSink> inner_;` になった

Java の `Display display;` は**参照**です。誰が解放するかを書く必要がありません。
C++ で `LogSink inner_;` と書くと**値**になり、これは抽象クラスなのでそもそもコンパイルできません。
仮に具象型だったとしても、代入した瞬間にスライシングして派生部分が消えます。

選択肢は 3 つです。

| 書き方 | 意味 | 誰が解放するか |
| --- | --- | --- |
| `std::unique_ptr<LogSink> inner_;` | 中身を**所有する** | このデコレータ |
| `LogSink & inner_;` | 中身を**借りる** | 呼び出し側 |
| `std::shared_ptr<LogSink> inner_;` | 中身を**共有する** | 最後の 1 人 |

**この章は 1 番で書きます。** Decorator は「包んだら、包んだ側が中身の面倒を見る」のが
自然だからです。外側 1 個を捨てれば入れ子が丸ごと消えます。

2 番（参照で持つ）も書けます。ただし、

```cpp
std::unique_ptr<LogSink> build()
{
  PlainMessage plain;              // ローカル変数
  return std::make_unique<LevelTag>(plain, "INFO");   // 参照で持つ版だとこう書ける
}                                  // plain はここで死ぬ。返った先は死んだ中身を指す
```

**寿命を守る責任が呼び出し側に残ります。** 型には何も書かれません。
第1章のイテレータと同じ問題です。この講習では、迷ったら所有する側に倒します。

### 変更点2: `protected Display display;` を `private` + `protected` アクセサにした

Java 版は `display` を `protected` にして派生から直接触らせます。C++ でも書けますが、
`protected` なメンバ変数は派生クラス全部から書き換えられるので、
`inner_` を差し替えられたり `nullptr` にされたりする余地が残ります。

**メンバ変数は `private`、必要なら `protected` の const アクセサ**にします。
派生が欲しいのは「中身を読むこと」だけです。

### 変更点3: `format()` に `const` を付けた

整形は状態を変えません。Java にはこの区別がないので書き忘れます。
`const` を付けておくと、`const LogSink &` として受け取ったものも整形できます。

## 12.2 誰が所有するのか — コンストラクタは値で受けて `std::move`

`unique_ptr` を受け取るコンストラクタの書き方は、実質これ一択です。

```cpp
explicit SinkDecorator(std::unique_ptr<LogSink> inner)   // 値で受ける
: inner_(std::move(inner))                               // move でメンバへ
{
}
```

**値で受ける**のがポイントです。`unique_ptr` はコピーできないので、
呼び出し側は `std::move` するか一時オブジェクトを渡すしかありません。
つまり **「所有権を渡してください」というメッセージが引数の型に書かれている**ことになります。

初期化子リストで `inner_(inner)` と書くとコンパイルエラーです。

```
error: call to implicitly-deleted copy constructor of 'std::unique_ptr<LogSink>'
```

`const std::unique_ptr<LogSink> &` で受けるのは**間違い**です。
借りるだけなら `unique_ptr` を経由する意味がありません（`LogSink &` で足ります）。

## 12.3 入れ子の組み立てがムーブの連鎖になる

所有する設計を選ぶと、組み立てはこうなります。

```cpp
auto sink = std::make_unique<TimestampTag>(
              std::make_unique<LevelTag>(
                std::make_unique<PlainMessage>(), "INFO"), "12:00:00");
```

Java 版の

```java
Display d = new SideBorder(new FullBorder(new StringDisplay("Hello")), '*');
```

と構造は同じですが、`std::make_unique<...>` が毎回挟まるので**読めません**。
3 重で既にこれです。

対処は簡単で、**包む操作を関数にします**。

```cpp
std::unique_ptr<LogSink> with_level(std::unique_ptr<LogSink> inner, std::string level)
{
  return std::make_unique<LevelTag>(std::move(inner), std::move(level));
}
```

呼ぶ側はこうなります。

```cpp
auto sink = with_level(with_timestamp(plain(), "12:00:00"), "INFO");
```

**中を通っているのは全部ムーブです。** `unique_ptr` が値で渡り、値で返り、
最後にいちばん外側の `unique_ptr` 1 個だけが手元に残ります。
この 1 個を捨てれば、入れ子が全部消えます。

## 12.4 Decorator は Composite の特殊形

第11章の `Composite` と比べてください。

| | Composite | Decorator |
| --- | --- | --- |
| 子の数 | 0 個以上（`std::vector<std::unique_ptr<Component>>`） | **ちょうど 1 個**（`std::unique_ptr<Component>`） |
| 目的 | 全体と部分を同一視する | 機能を後から足す |
| 再帰 | 木 | 一本道の鎖 |

**構造としては、Decorator は「子が 1 つの Composite」です。**
だから所有権の扱い方も同じで、`vector` が `unique_ptr` 1 個に減っただけです。
第11章で `unique_ptr` の `vector` を書けたなら、ここは新しいことがほとんどありません。

逆に言うと、**第11章で詰まったところは、ここでも同じところで詰まります。**
コピーできない、`push_back` に `std::move` が要る、といった話です。

## 12.5 C++ 固有の危険 — 仮想デストラクタが無いと内側が解放されない

基底クラスの `virtual ~LogSink() = default;` を消すとどうなるか、実際にやりました。

```cpp
class Sink
{
public:
  ~Sink() {}                                   // virtual を書き忘れた
  virtual std::string format(const std::string & m) const = 0;
};
```

Apple clang はコンパイル時点で警告します（`-Wall -Wextra -Wpedantic`）。

```
warning: delete called on 'Sink' that is abstract but has non-virtual destructor
         [-Wdelete-abstract-non-virtual-dtor]
warning: delete called on non-final 'Border' that has virtual functions but
         non-virtual destructor [-Wdelete-non-abstract-non-virtual-dtor]
```

そして手元で実行すると、`format()` の出力すら出ないまま終了コード 133 で落ちました。
**未定義動作なので、何が起きるかは環境によって変わります。**
運が良ければ落ち、悪ければ黙って内側だけ漏れ続けます。

理屈はこうです。外側は `std::unique_ptr<Sink>` で持たれているので、
解放時に呼ばれるのは `~Sink()` だけです。`~Border()` が呼ばれない
= **`Border` のメンバである `inner_`（内側全部）のデストラクタが走らない**。
入れ子が深いほど、一発で全部漏れます。

Decorator は入れ子が本体なので、**このパターンで仮想デストラクタを忘れる被害はいちばん大きい**です。

正しく書いたときの破棄の順番も見ておきます（12.6 の実測です）。

```
~Border([INFO])
~Border(12:00:00)
~Plain
```

**外側から内側へ**順に呼ばれます。外側のデストラクタ本体が走り、
そのあとメンバ（`inner_`）が破棄されるからです。

## 12.6 標準ライブラリ／言語機能に同じものが無いか

**あります。しかも中心的なところにあります。**

- **`std::istream` / `std::ostream` と `std::streambuf`**
  ストリームは「文字列の整形」と「実際の入出力先」を分けています。
  `std::ofstream` も `std::ostringstream` も、`std::ostream` の口は同じで
  中の `streambuf` が違うだけです。自作の `streambuf` を挟んで
  「行頭にタイムスタンプを足す」といった加工を入れるのは、まさに Decorator です。
- **`std::unique_ptr` のデリータ**
  型パラメータで振る舞いを差し替える、コンパイル時の Decorator と見ることもできます。
- **C++20 の `<ranges>` の view**
  `v | std::views::filter(...) | std::views::transform(...)` は、
  範囲を範囲で包んでいく Decorator です。**この講習は C++17 なので使いません**が、
  「包むと同じインタフェースのまま機能が足される」という形は同じです。

つまり **Decorator は C++ で自作する場面が比較的少ないパターン**です。
「同じインタフェースのまま機能を足したい」と思ったら、
まず `streambuf` やテンプレートで済まないかを考えてください（→ [0. 使う前に](00_使う前に.md)）。

## 12.7 手元で試す

1 ファイルで完結します。**出力を予想してから**実行してください。

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Sink
{
public:
  virtual ~Sink() = default;
  virtual std::string format(const std::string & m) const = 0;
};

class Plain : public Sink
{
public:
  ~Plain() override { std::cout << "~Plain\n"; }
  std::string format(const std::string & m) const override { return m; }
};

class Border : public Sink
{
public:
  Border(std::unique_ptr<Sink> inner, std::string tag)
  : inner_(std::move(inner)), tag_(std::move(tag))
  {
  }
  ~Border() override { std::cout << "~Border(" << tag_ << ")\n"; }

  std::string format(const std::string & m) const override
  {
    return tag_ + " " + inner_->format(m);
  }

private:
  std::unique_ptr<Sink> inner_;
  std::string tag_;
};

std::unique_ptr<Sink> wrap(std::unique_ptr<Sink> inner, std::string tag)
{
  return std::make_unique<Border>(std::move(inner), std::move(tag));
}

int main()
{
  {
    auto a = std::make_unique<Border>(
      std::make_unique<Border>(std::make_unique<Plain>(), "[INFO]"), "12:00:00");
    std::cout << a->format("moving") << "\n";
  }
  std::cout << "----\n";
  {
    auto b = wrap(wrap(std::make_unique<Plain>(), "12:00:00"), "[INFO]");
    std::cout << b->format("moving") << "\n";
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 2 つのブロックの整形結果は同じか。デストラクタは何個呼ばれるか</summary>

```
12:00:00 [INFO] moving
~Border(12:00:00)
~Border([INFO])
~Plain
----
[INFO] 12:00:00 moving
~Border([INFO])
~Border(12:00:00)
~Plain
```

- **整形結果は違います。** 包む順番が逆なので、タグの順番も逆になります。
  これが Decorator の本質で、同じ部品でも**組み立て順が結果を決めます**
- デストラクタは各ブロックで 3 個。`unique_ptr` を 1 個捨てただけで
  内側まで到達しています。**外側から内側へ**の順です
- 下のブロックは `wrap()` を使って `make_unique` の入れ子を消したもの。
  引数の並びが「内側から外側へ」読める形になります

</details>

## 12.8 マイコンでの結論

**`unique_ptr` の入れ子は使えません。** 包む数だけヒープ確保が走ります。
起動時に 1 回だけ組んでずっと使うならまだしも、
状況に応じて組み替えるコードをループの中に書くと、断片化していずれ確保に失敗します。

代わりに **型で入れ子にします**。`Border<Border<Text>>` です。

```cpp
#include <cstdio>
#include <cstring>
#include <type_traits>

// 出力先。固定長バッファだけ。動的確保なし。
class Buffer
{
public:
  void put(const char * text)
  {
    const std::size_t n = std::strlen(text);
    if (used_ + n >= sizeof(data_)) {
      return;                       // 溢れたら捨てる。例外は投げない
    }
    std::memcpy(data_ + used_, text, n);
    used_ += n;
    data_[used_] = '\0';
  }

  const char * c_str() const { return data_; }

private:
  char data_[128] = {};
  std::size_t used_ = 0;
};

// Component。仮想関数なし。
class PlainMessage
{
public:
  void emit(Buffer & out, const char * message) const { out.put(message); }
};

// Decorator。中身を「値で」持つ。型で入れ子にする。
template <typename Inner>
class TagDecorator
{
public:
  constexpr TagDecorator(Inner inner, const char * tag)
  : inner_(inner), tag_(tag)
  {
  }

  void emit(Buffer & out, const char * message) const
  {
    out.put(tag_);
    out.put(" ");
    inner_.emit(out, message);      // 仮想呼び出しではない。インライン展開される
  }

private:
  Inner inner_;
  const char * tag_;
};

template <typename Inner>
constexpr TagDecorator<Inner> with_tag(Inner inner, const char * tag)
{
  return TagDecorator<Inner>(inner, tag);
}

int main()
{
  const auto sink = with_tag(with_tag(PlainMessage{}, "12:00:00.000"), "[INFO]");

  static_assert(!std::is_polymorphic<decltype(sink)>::value, "vtable があってはいけません");
  // 中身はタグ2本のポインタと、空クラス PlainMessage の1バイト分の詰め物だけ。
  static_assert(sizeof(sink) <= 3 * sizeof(const char *), "余計なものが入っています");

  Buffer out;
  sink.emit(out, "moving");
  std::printf("%s\n", out.c_str());
  std::printf("sizeof(sink) = %zu\n", sizeof(sink));
  return 0;
}
```

マイコンの設定に近づけてビルドして、実行しました。

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti micro.cpp -o micro && ./micro
```

```
[INFO] 12:00:00.000 moving
sizeof(sink) = 24
```

**確認できること**

- `-fno-exceptions -fno-rtti` で警告ゼロ・エラーゼロで通る
- `std::is_polymorphic` が `false` = **vtable が無い**。ROM も RAM も食わない
- ヒープ確保はゼロ。`sink` はスタック（この例では 24 バイト）に丸ごと乗る
- `inner_.emit(...)` は仮想呼び出しではないので、最適化でインライン展開できる

**代償**は 2 つです。

1. **実行時に組み替えられません。** 型が組み立て順を持っているので、
   「設定ファイルの内容で包む順番を変える」ができない
2. 組み合わせの数だけコードが生成される。3 種類のデコレータを 3 重にすると、
   使った組み合わせの分だけ `emit()` の実体が増えます

実行時の組み替えが本当に要るのか、を先に確認してください。
部活のログ整形なら、**ビルド時に決まっていることがほとんど**です。

## 12.9 ROS 2 での結論（補足）

Linux 上なら `unique_ptr` の入れ子で問題ありません。起動時に 1 回組むだけです。

rclcpp に GoF 版の Decorator クラスは出てきませんが、
`rclcpp::PublisherBase` を包んで統計を取る、といった自作は素直に書けます。
ただし ROS 2 のログ（`RCLCPP_INFO`）は既にマクロ側で時刻・ノード名・重大度を付けるので、
**そこを Decorator で作り直す必要はありません**。

## 12.10 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: call to implicitly-deleted copy constructor of 'std::unique_ptr<...>'` | 初期化子リストで `inner_(inner)` と書いた。`std::move(inner)` にする |
| `warning: delete called on ... non-virtual destructor` | 基底に `virtual ~LogSink() = default;` が無い |
| 外側を捨てたのに内側のデストラクタが呼ばれない | 同上。入れ子が丸ごと漏れている |
| タグの順番が思ったのと逆 | `format()` の中で中身を呼ぶ順序。**先に中身へ整形させてから**包む |
| 中身を差し替えたら二重解放した | `unique_ptr` を生ポインタ経由で渡している。所有権は `std::move` でだけ動かす |
| `make_unique` の入れ子が読めない | `with_xxx()` ヘルパを書く（12.3） |
| 参照で持つ版にしたら実行時に落ちた | 中身がローカル変数で、外側より先に死んでいる |
| テンプレート版で `sizeof` が思ったより大きい | 空クラスもメンバとして持てば 1 バイト以上取り、アラインで詰められる |

## 12.11 対応する課題

```bash
./drill run dp12
```

`exercises/dp12_decorator/src/log_sink.cpp` に、

1. `join_tag()` — タグと本文の連結（`unique_ptr` 版とテンプレート版で共有）
2. `PlainMessage` — いちばん内側。デストラクタで破棄を記録
3. `SinkDecorator` — 中身を `std::move` で受け取って所有する
4. `LevelTag` / `TimestampTag` / `SourceTag` — 3 種類のデコレータ
5. `with_level()` / `with_timestamp()` / `with_source()` — 組み立てヘルパ

を実装します。テストは、**包む順番を変えると出力が変わること**、
**外側 1 個を捨てれば内側まで破棄されること**（デストラクタの記録を照合します）、
**テンプレート版が同じ出力を返し、かつ `std::is_polymorphic_v` が `false` であること**を見ます。

## 12.12 この章のまとめ

- Java の `Display display;` は、C++ では **所有（`unique_ptr`）／借用（`&`）／共有（`shared_ptr`）** の選択になる
- Decorator は所有する版が素直。**外側 1 個を捨てれば入れ子が全部消える**
- コンストラクタは `std::unique_ptr<T>` を**値で受けて `std::move`**。所有権の移動が型に書かれる
- 組み立ては**ムーブの連鎖**。`make_unique` の入れ子は読めないので、包む関数を作る
- **Decorator は子が 1 つの Composite**（第11章）。所有権の扱いは同じ
- **仮想デストラクタを忘れると入れ子の内側が丸ごと解放されない。** このパターンで被害が最大
- 標準ライブラリでは `streambuf` と（C++20 の）`views` が Decorator。自作する前に探す
- マイコンでは `unique_ptr` の入れ子は使わない。**テンプレートで型に入れ子を作る**とヒープゼロ・vtable ゼロ

---

前: [11. Composite](11_Composite.md) ／ 次: 13. Visitor（準備中）
