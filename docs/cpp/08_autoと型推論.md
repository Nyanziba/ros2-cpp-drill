# 8. `auto` と型推論

> **この章のねらい**: `auto` はドリルに **139 回**出てきます。全構文で最多です。
> それでいて、`auto` は**書き手が型を書かなくて済む機能ではありません。**
> コンパイラが型を決める規則が明確にあり、その規則を知らないと
> 「`auto` にしたらコピーが発生していた」「`auto` にしたら `const` が消えた」という事故が起きます。
> この章は短いですが、4 章と 5 章の内容を `auto` の文脈で確定させます。

> **前提**: [入門編 6章 `const`](../cpp-basics/06_const.md)と
> [3章 参照](../cpp-basics/03_参照.md)を読んでいる前提です。
> **`auto` は `const` と `&` を落とします。** 元の型が読めないと、
> 落ちたことに気付けず、意図しないコピーが生まれます。

## 8.1 なぜ `auto` を使うのか

rclcpp の型名は長いです。

```cpp
std::shared_ptr<rclcpp::Publisher<std_msgs::msg::String, std::allocator<void>>> publisher =
  this->create_publisher<std_msgs::msg::String>("topic", 10);
```

これを `auto` で書きます。

```cpp
auto publisher = this->create_publisher<std_msgs::msg::String>("topic", 10);
```

100 文字の行長制限（[コーディング規約](../ros2-コーディング規約.md)）を守るには、`auto` が実質必須です。

**`auto` は「型を決めない」のではなく「型を書かない」だけです。**
型は依然として静的に 1 つに決まります。Python の変数とは全く違います。

`auto` の 3 つの価値を挙げます。

1. **長い型名を書かない** — 上の例
2. **書けない型を扱える** — ラムダの型には名前がありません（7 章）
3. **型が変わったときに直す箇所が減る** — 戻り値型が変わっても呼び出し側はそのまま

課題 01 の実例です。

```cpp
// solutions/01_publisher/src/minimal_publisher.cpp
auto message = std_msgs::msg::String();
```

これは `std_msgs::msg::String message;` とほぼ同じです（`auto` 版は明示的に
コンストラクタを呼んでいます）。公式チュートリアルがこう書いているので合わせています。

## 8.2 `auto` は参照と `const` を落とす

**これが `auto` の唯一の重要な落とし穴です。**

```cpp
#include <iostream>
#include <string>
#include <vector>

int main()
{
  std::vector<std::string> v{"hello", "world"};

  const std::string & ref = v[0];

  auto a = ref;          // std::string（コピー！const も & も落ちる）
  auto & b = ref;        // const std::string &（参照のまま）
  const auto & c = ref;  // const std::string &（明示的で読みやすい）

  a = "changed";         // コピーなので v[0] は変わらない
  std::cout << v[0] << " " << a << "\n";
  return 0;
}
```

```
hello changed
```

**`auto a = ref;` は `std::string` になり、コピーが発生します。**
`ref` が参照であることも `const` であることも、`auto` は引き継ぎません。

規則は 1 行で言えます。**`auto` は「値」として推論する。**
参照や `const` が欲しければ、自分で `&` や `const` を書き足す。

| 書き方 | 推論される型 | コピーは |
| --- | --- | --- |
| `auto x = expr;` | 値（`const` と `&` が落ちる） | **起きる** |
| `auto & x = expr;` | 参照（`const` は保たれる） | 起きない |
| `const auto & x = expr;` | `const` 参照 | 起きない |
| `auto && x = expr;` | 転送参照（右辺値も左辺値も受ける） | 起きない |

### 範囲 for でこれが効く

4 章でも触れましたが、ここが実務でいちばん出ます。

```cpp
std::vector<std::string> messages = /* 大きなデータ */;

for (auto s : messages) { }         // 毎周コピー
for (auto & s : messages) { }       // 参照（書き換えられる）
for (const auto & s : messages) { } // 参照（読むだけ）← 既定にすべき形
```

4 章の `copies.cpp` で実測したとおり、`auto x` は 1 要素あたり 1 回コピーします。
要素が `sensor_msgs::msg::Image` なら、1 周ごとに 900KB のコピーです。

**`const auto &` を既定にしてください。**
書き換えたいときだけ `auto &`、本当にコピーが欲しいときだけ `auto`。

ドリルには `const auto` が 26 箇所あります。ほとんどが範囲 for です。

```cpp
// tools/drill_harness.hpp などのパターン
for (const auto & log : captured_logs) {
  // ...
}
```

### `auto` でコピーが起きて困る典型例

```cpp
auto msg = subscription_msg;         // shared_ptr のコピー（カウント +1、アトミック操作）
const auto & msg = subscription_msg; // 参照（カウントはそのまま）
```

`shared_ptr` のコピーは 6 章のとおりアトミックなカウント増減を伴います。
高頻度で回るループでは無視できない差になります。

## 8.3 `auto` を使わないほうがよい場所

**「型が読み手にとって重要な情報」のときは書いてください。**

```cpp
auto count = get_count();       // int? size_t? double?
size_t count = get_count();     // 読めば分かる
```

特に危ないのが整数型です。

```cpp
auto n = v.size();              // size_t（符号なし）
auto diff = a.size() - b.size();  // size_t の減算 → 負にならず巨大な値になる
```

```cpp
#include <iostream>
#include <vector>

int main()
{
  std::vector<int> a{1, 2};
  std::vector<int> b{1, 2, 3, 4, 5};
  auto diff = a.size() - b.size();
  std::cout << diff << "\n";
  return 0;
}
```

```
18446744073709551613
```

`2 - 5 = -3` のはずが、`size_t` は符号なしなので**巨大な正の値に回り込みます。**

`auto` のせいというより符号なし整数の性質ですが、
**`auto` は「符号なしである」ことを隠してしまう**のが問題です。

```cpp
std::ptrdiff_t diff = static_cast<std::ptrdiff_t>(a.size()) - static_cast<std::ptrdiff_t>(b.size());
```

`-Wall -Wextra` は `-Wsign-compare` でこの類を警告しますが、
上の減算のケースは警告が出ません。**サイズの差を取るときは意識してください。**

### `auto` と初期化子リスト

```cpp
auto a = 1;        // int
auto b = 1.0;      // double
auto c = {1, 2};   // std::initializer_list<int>（!）
auto d{1};         // int（C++17 以降）
```

`auto c = {1, 2};` が `std::initializer_list` になるのは覚えておく価値があります。
2 章の `std::vector<int> b{3, 0};` の話と同じ系統の罠です。

## 8.4 構造化束縛（C++17）

`auto` の親戚で、複数の値を一度に受け取れます。

```cpp
#include <iostream>
#include <map>
#include <string>

int main()
{
  std::map<std::string, int> params{{"speed", 10}, {"accel", 3}};

  for (const auto & [name, value] : params) {
    std::cout << name << " = " << value << "\n";
  }
  return 0;
}
```

```
accel = 3
speed = 10
```

これがなかった時代は `it->first` / `it->second` と書いていました。
**`const auto & [a, b]` の形もコピーを避けられる**ので、範囲 for と同じ原則が効きます。

`std::pair` を返す関数の受け取りにも使えます。

```cpp
auto [ok, value] = try_get_parameter("speed");
```

ドリルには構造化束縛は出てきませんが、C++17 のコードを読むときに必要です。

## 8.5 `decltype` — 式の型を取る

`auto` は「初期化式から型を決める」ものですが、
`decltype` は「式の型そのもの」を取ります。

```cpp
int a = 1;
const int & r = a;

auto x = r;              // int（const と & が落ちる）
decltype(r) y = a;       // const int &（そのまま）
```

**`decltype` は落としません。**
テンプレートを書くときに必要になりますが、ドリルでは出てきません。
`auto` との違いだけ知っておけば十分です。

`decltype(auto)` という書き方もあり、「`auto` の書き方で `decltype` の規則を使う」という意味です。
ライブラリの実装以外で使う場面はほぼありません。

## 手元で試す

**`auto` でコピーが起きていることを、実測で確認します。**

```cpp
// autocopy.cpp
#include <iostream>
#include <string>
#include <vector>

struct Big
{
  std::string tag;
  std::vector<int> payload;

  explicit Big(std::string t) : tag(std::move(t)), payload(1000, 0) {}
  Big(const Big & o) : tag(o.tag), payload(o.payload)
  {
    std::cout << "  [copy " << tag << "]\n";
  }
  Big & operator=(const Big &) = default;
};

int main()
{
  std::vector<Big> items;
  items.reserve(3);                 // 再確保によるコピーを防ぐ
  items.emplace_back("a");
  items.emplace_back("b");
  items.emplace_back("c");

  std::cout << "-- for (auto x : items)\n";
  for (auto x : items) { (void)x; }

  std::cout << "-- for (auto & x : items)\n";
  for (auto & x : items) { (void)x; }

  std::cout << "-- for (const auto & x : items)\n";
  for (const auto & x : items) { (void)x; }

  std::cout << "-- auto y = items[0]\n";
  auto y = items[0];
  (void)y;

  std::cout << "-- const auto & z = items[0]\n";
  const auto & z = items[0];
  (void)z;

  std::cout << "-- 符号なしの罠\n";
  std::vector<int> p{1, 2};
  std::vector<int> q{1, 2, 3, 4, 5};
  auto diff = p.size() - q.size();
  std::cout << "  auto diff = " << diff << "\n";
  std::cout << "  正しく = "
            << static_cast<std::ptrdiff_t>(p.size()) - static_cast<std::ptrdiff_t>(q.size())
            << "\n";
  return 0;
}
```

**予想: `[copy ...]` は合計何回出るか。どの行から出るか。**

```bash
g++ -std=c++17 -Wall -Wextra autocopy.cpp -o autocopy && ./autocopy
```

3 点確認してください。

1. `for (auto x : items)` で 3 回、`auto &` と `const auto &` で 0 回
2. `auto y = items[0]` で 1 回、`const auto & z` で 0 回
3. `auto diff` が `18446744073709551613` になる

`items.reserve(3)` を消してもう一度実行してください。
**`emplace_back` のたびに再確保が起きて、`[copy]` が増えます。**
`std::vector` は容量を超えると新しい領域を確保して全要素を移すので、
`Big` にムーブコンストラクタが無い（コピーコンストラクタを書いたので
自動生成されない。5 章の Rule of Five）ためコピーになります。

**5 章の表を思い出してください。** `Big(const Big &)` を書いた時点で
ムーブコンストラクタが生成されなくなっています。
`Big(Big &&) noexcept = default;` を足すと `[copy]` が消えることを確認してください。

## つまずきポイント

**`auto` にしたら中身が変わらない**
`auto x = container[i];` でコピーを受け取っています。`auto & x` にしてください。

**`error: assignment of read-only reference ‘x’`（範囲 for の中で）**
`const auto &` で受けているのに書き換えようとしています。`auto &` にしてください。

**`auto` の変数に `nullptr` を入れたい**
`auto p = nullptr;` は `std::nullptr_t` という専用の型になり、あとから代入できません。
型を書いてください。

**`error: unable to deduce ‘auto’ from ...`**
初期化式が無いか、推論できない形です。`auto x;` は書けません。

**符号なし整数の比較で警告が出る**

```
warning: comparison of integer expressions of different signedness [-Wsign-compare]
```

`int i` と `v.size()` を比較しています。`size_t i` にするか、
`static_cast` で明示的に揃えてください。ドリルの全課題で
`-Wall -Wextra` が有効なので、これは実際に出ます。

**`auto` が `std::initializer_list` になった**
`auto c = {1, 2};` の形です。`auto c = std::vector<int>{1, 2};` のように型を書いてください。

## ドリルのどこで出るか

| 課題 | この章の何が出るか |
| --- | --- |
| 01 | `auto message = std_msgs::msg::String();` |
| 01, 02 | `auto` で `create_publisher` / `create_subscription` の戻り値を受ける形も可（ただしメンバに持つこと。6 章） |
| 04, 05 | `auto request = std::make_shared<AddTwoInts::Request>();` |
| 05 | `auto future = client_->async_send_request(request);` — future の型は長いので `auto` が実用的 |
| 10 | `auto result = std::make_shared<Fibonacci::Result>();` |
| 13 | `auto future = client_->async_send_request(request);` |
| 14 | `auto msg = std::make_unique<std_msgs::msg::String>();` |
| テスト | `const auto &` が 26 箇所。ほとんど範囲 for |

課題 05 が `auto` の必要性がいちばん分かる例です。
`async_send_request` の戻り値の型を手で書くとこうなります。

```cpp
std::shared_future<std::shared_ptr<example_interfaces::srv::AddTwoInts_Response_<std::allocator<void>>>>
```

100 文字を超えます。`auto` を使うしかありません。

```cpp
auto future = client_->async_send_request(request);
```

**この行は `auto` を「型を書かないための手抜き」として使っているのではなく、
「書けない・書くべきでない型」を扱うための正当な用途です。**

## 参考

- [ROS 2 のコーディング規約](../ros2-コーディング規約.md) — 行長 100 文字。`auto` が実質必須になる理由
- `cppreference` の [Placeholder type specifiers (auto)](https://en.cppreference.com/w/cpp/language/auto) と [Structured binding](https://en.cppreference.com/w/cpp/language/structured_binding)

---

前章 → [7. ラムダと `std::bind`](07_ラムダとstd_bind.md)
次章 → [9. テンプレートの読み方](09_テンプレートの読み方.md)
