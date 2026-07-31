# 4. 参照と const — rclcpp での使い分け

> **この章のねらい**: 課題 `cpp04` で書く関数のシグネチャはこれです。
>
> ```cpp
> void MinimalSubscriber::topic_callback(const std_msgs::msg::String & msg) const
> ```
>
> `const` と `&` の**読み方そのもの**は[入門編](../cpp-basics/README.md)で扱いました。
> この章は「読める」の次、**「rclcpp ではどれを選ぶか」**を扱います。
> コピーのコストを実測し、参照を返してはいけない場面を見て、
> 購読コールバックの引数型を 4 つの選択肢から選べるようにします。

## 4.1 前提 — 入門編で扱ったこと

**この章は、次の3つが読める前提で書いてあります。**
自信がなければ[入門編](../cpp-basics/README.md)に戻ってください。1〜2時間で埋まります。

| 前提 | 入門編のどこ |
| --- | --- |
| 宣言を右から左に読む手順（`const int * const p` を読み下す） | [1章 宣言を読む](../cpp-basics/01_宣言を読む.md) |
| 参照とは何か（別名、再束縛できない、null になれない） | [3章 参照](../cpp-basics/03_参照.md) |
| ポインタとは何か（`nullptr`、間接参照、参照との使い分け） | [4章](../cpp-basics/04_ポインタ1.md)・[5章](../cpp-basics/05_ポインタ2.md) |
| `const` が 4 か所で意味を変えること | [6章 const](../cpp-basics/06_const.md) |

手元に置いておくと便利な早見表だけ、ここに再掲します。

```cpp
void MinimalSubscriber::topic_callback(const std_msgs::msg::String & msg) const
//                                     ^^^^^ ①                          ^^^^^ ②
```

| 位置 | 意味 |
| --- | --- |
| ① 引数の `const &` | 借りるが書き換えない。コピーも起きない |
| ② `)` の後ろの `const` | このメンバ関数はメンバ変数を書き換えない |
| `const int * p` | 指す先が `const`。`p` は差し替えられる |
| `int * const p` | ポインタ自身が `const`。`*p` は書き換えられる |

**`const` は「自分のすぐ左」に付きます。** 左に何も無ければ右に付きます。
だから `const int *` と `int const *` は同じ意味です。

ここから先は、**この基礎を rclcpp でどう使い分けるか**の話です。

## 4.2 コピーのコストを測る

「値で渡すとコピーになる」と言われても、実感がないと `const &` を使う理由になりません。
測ってみます。

```cpp
#include <iostream>
#include <string>
#include <vector>

struct Tracked
{
  std::vector<int> data;

  Tracked() : data(1000, 0) { std::cout << "  [生成]\n"; }
  Tracked(const Tracked & other) : data(other.data) { std::cout << "  [コピー]\n"; }
  Tracked & operator=(const Tracked & other)
  {
    data = other.data;
    std::cout << "  [コピー代入]\n";
    return *this;
  }
};

void by_value(Tracked t) { (void)t; }
void by_const_ref(const Tracked & t) { (void)t; }

int main()
{
  std::cout << "生成:\n";
  Tracked t;

  std::cout << "by_value(t):\n";
  by_value(t);

  std::cout << "by_const_ref(t):\n";
  by_const_ref(t);

  return 0;
}
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/1148bvT6T)

```
生成:
  [生成]
by_value(t):
  [コピー]
by_const_ref(t):
```

**`by_const_ref` は何も出力していません。** コピーが 1 回も起きていない、という意味です。
`data` は `int` 1000 個なので 4KB。値渡しは毎回 4KB のヒープ確保とコピーをしています。

ROS のメッセージで考えると、`sensor_msgs::msg::Image` は VGA でも約 900KB、
`PointCloud2` は数 MB になります。
**購読コールバックが 30Hz で呼ばれるなら、値渡しは毎秒 27MB のコピー**です。
これが `const &` を使う理由です。

### 何を `const &` にすべきか

目安は単純です。

| 型 | 渡し方 |
| --- | --- |
| `int` / `double` / `bool` / ポインタ / `enum` | **値で渡す**（`const &` より速い） |
| `std::string` / `std::vector` / ROS のメッセージ / 自作クラス | **`const &` で渡す** |
| 所有権を渡したい | `std::unique_ptr` を値で（5章・6章） |

`int` を `const int &` にするのは逆効果です。
`int` は 4 バイトでレジスタに乗りますが、参照はアドレス（8 バイト）を渡して
参照先を読むという間接参照が入ります。**小さい型は値のほうが速い**のです。

境目は「ポインタ 2 個分（16 バイト）くらいまでは値」と覚えておけば実用上足ります。

課題 11 の `limit_velocity` が全部 `double` の値渡しなのは、この判断です。

```cpp
double limit_velocity(double target, double previous, double max_speed, double max_delta);
```

## 4.3 ダングリング参照 — 参照を返してはいけない場合

参照は「別名」なので、**元のものが死んだら参照も無効になります。**

```cpp
#include <iostream>
#include <string>

const std::string & bad()
{
  std::string local = "危険";
  return local;             // local はこの関数を抜けると消える
}

int main()
{
  const std::string & r = bad();
  std::cout << r << "\n";   // 未定義動作
}
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/8M9zKKbfd)

g++ は警告します。

```
warning: reference to local variable ‘local’ returned [-Wreturn-local-addr]
```

**警告で済んでしまう**のが怖いところです。実行すると、たまたま動くこともあります。

メンバへの参照を返すのは正しい使い方です。オブジェクトが生きている限り有効です。

```cpp
const std::string & name() const { return name_; }   // OK
```

ただし呼び出し側が持ち続けると危険です。

```cpp
const std::string & n = make_node()->name();   // ノードが即座に死ぬ
```

**参照を長く保持しない。** 使うのはその場だけにして、
持ち続けたければコピーするか `shared_ptr` にします（6章）。
rclcpp が何でも `shared_ptr` を返すのは、この判断を型で強制するためでもあります。

### `const auto &` は安全側の既定値

範囲 for でよく使う形です。

```cpp
for (const auto & s : sensors) {
  s->report();
}
```

`const auto &` は「コピーせず、書き換えない」なので、
**迷ったらこれ**にしておけば損がありません（8章で `auto` を扱います）。

`for (auto s : sensors)` と書くと要素をコピーします。
`std::unique_ptr` の場合はコピーできないのでコンパイルエラーになり、
気付けますが、`std::string` の場合は静かにコピーされます。

## 4.4 rclcpp のコールバック引数はどれを選ぶか

購読コールバックの引数には複数の形が使えます。ドリルでは 2 つ出てきます。

```cpp
// 課題 02
void topic_callback(const std_msgs::msg::String & msg) const;

// 課題 14
void topic_callback(std_msgs::msg::String::ConstSharedPtr msg);
```

選び方の基準は **「そのメッセージをどう扱いたいか」** です。

| 引数の型 | 使う場面 | コピーは |
| --- | --- | --- |
| `const T & msg` | その場で読むだけ | 起きない |
| `T::ConstSharedPtr msg` | 保持したい / ゼロコピーを効かせたい | 起きない |
| `T::SharedPtr msg` | 書き換えたい（他の購読者と共有） | 起きうる |
| `T::UniquePtr msg` | 書き換えたい（単独所有） | 起きうる |

**`const T &` でもコピーは起きません。** これは実測で確認できることで、
プロセス内通信が有効なとき、送信側と受信側でメッセージのアドレスが一致します。

「値で受けるとコピーになる」という説明を見かけますが、
`any_subscription_callback.hpp` は値で受ける形をそもそもサポートしていないので、
この心配は不要です。

保持したいなら `ConstSharedPtr` です。`const T &` を保持しようとすると
4.3 節のダングリング参照になります。

## 手元で試す

**コピーが何回起きているかを数えます。**

```cpp
// copies.cpp
#include <iostream>
#include <string>
#include <vector>

struct Msg
{
  std::string data;

  Msg() = default;
  explicit Msg(std::string s) : data(std::move(s)) {}
  Msg(const Msg & o) : data(o.data) { std::cout << "  copy\n"; }
  Msg & operator=(const Msg & o) { data = o.data; std::cout << "  copy=\n"; return *this; }
};

Msg g_last;

void log_only(const Msg & m)      { std::cout << "  read: " << m.data << "\n"; }
void log_value(Msg m)             { std::cout << "  read: " << m.data << "\n"; }
void keep_it(const Msg & m)       { g_last = m; }          // 保持するのでコピーが必要

int main()
{
  Msg m("hello");

  std::cout << "log_only(m):\n";  log_only(m);
  std::cout << "log_value(m):\n"; log_value(m);
  std::cout << "keep_it(m):\n";   keep_it(m);

  std::vector<Msg> v;
  v.push_back(m);
  std::cout << "-- 範囲 for（コピーする版）\n";
  for (auto x : v) { (void)x; }
  std::cout << "-- 範囲 for（const auto &）\n";
  for (const auto & x : v) { (void)x; }
  return 0;
}
```

**予想: `copy` と `copy=` は合計何回出るか。どの行から出るか。**

```bash
g++ -std=c++17 -Wall -Wextra copies.cpp -o copies && ./copies
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/ojqrM6M81)

```
log_only(m):
  read: hello
log_value(m):
  copy
  read: hello
keep_it(m):
  copy=
  copy
-- 範囲 for（コピーする版）
  copy
-- 範囲 for（const auto &）
```

4 点確認してください。

1. **`log_only` は 0 回、`log_value` は 1 回。** 同じ `m` を同じように読んでいるだけで、
   `&` の 1 文字がコピーの有無を決めています
2. `keep_it` は `const &` で受けているのに `copy=` が出ています。
   → **保持するなら、どこかで必ずコピーが必要。** `const &` は「渡すとき」を無料にするだけで、
   「保存するとき」は無料になりません
3. `keep_it` の直後の `copy` は `keep_it` のものではなく、次の行の
   `v.push_back(m)` のものです。**出力の並びに引きずられて原因を誤認しないよう注意してください。**
   紛らわしければ `push_back` の前に `std::cout` を 1 行入れて確かめてください
4. 範囲 for は `auto x` で 1 回コピー、`const auto & x` で 0 回

次に `const` を破ってエラーを見ます。`log_only` の中身を次に変えてください。

```cpp
void log_only(const Msg & m) { m.data = "書き換え"; }
```

エラーメッセージ（`assignment of member ... in read-only object`）を確認します。

最後にダングリング参照を作ってみます。

```cpp
const std::string & dangling()
{
  std::string local = "危険";
  return local;
}
```

`main` から呼んで出力してください。
**警告が 1 つ出るだけでコンパイルは通ります。**
実行結果が実行するたびに変わるか（変わらないか）も見てください。

## つまずきポイント

**`error: binding reference of type ‘T&’ to ‘const T’ discards qualifiers`**
`const` なものを非 `const` 参照で受けようとしています。
受け側を `const T &` にするか、本当に書き換えたいなら呼び出し側の `const` を外します。

**`error: passing ‘const Foo’ as ‘this’ argument discards qualifiers`**
`const` メンバ関数の中から、非 `const` のメンバ関数を呼んでいます。
呼ばれる側にも `const` を付けるか、呼ぶ側の `const` を外します。
**「`const` が足りない」と言われたら、まず呼ばれる側に付けられないか考えてください。**

**`error: cannot bind non-const lvalue reference to an rvalue`**

```cpp
void f(std::string & s);
f("hello");           // エラー
f(std::string("hi")); // エラー
```

```
error: cannot bind non-const lvalue reference of type ‘std::string&’ to an rvalue of type ‘std::string’
```

一時オブジェクト（右辺値）は非 `const` 参照に束縛できません。
`const std::string & s` なら通ります。**これも「`const &` を既定にする」理由の 1 つ**です。
右辺値も左辺値も受けられるようになります。

この「右辺値」という言葉が次章の主役です。

**`const` を付けたらメンバをキャッシュできなくなった**
`mutable` を付けたメンバは `const` メンバ関数からでも書き換えられます。

```cpp
mutable std::mutex mutex_;   // よく使われる正当な例
```

ロックは「論理的には状態を変えていない」ので `mutable` が適切です。
それ以外で `mutable` を使いたくなったら、設計を見直す合図です。

## ドリルのどこで出るか

| 課題 | この章の何が出るか |
| --- | --- |
| 02 | `void topic_callback(const std_msgs::msg::String & msg) const` — `const` が 2 つ、意味が違う |
| 04, 05 | サービスの `Request::SharedPtr` / `Response::SharedPtr`。**`const` が付いていない**（response は書き換えるため） |
| 06, 07 | `get_parameter` の戻り値を受ける形 |
| 11 | `limit_velocity(double, double, double, double)` — 全部値渡し。小さい型は値が正しい |
| 12 | QoS オブジェクトをメソッドチェーンで組む |
| 13 | `const std_msgs::msg::Int32 & msg` |
| 14 | `ConstSharedPtr` — 保持したいので参照ではなくスマートポインタ |
| 14, 15 | `const rclcpp::NodeOptions & options` — 大きい設定オブジェクトなので `const &` |

課題 04 のサービスコールバックを見ると、この章の判断が見えます。

```cpp
void add_two_ints(
  const std::shared_ptr<AddTwoInts::Request> request,
  std::shared_ptr<AddTwoInts::Response> response)
```

**`request` には `const` があり、`response` には無い**。
リクエストは読むだけ、レスポンスは書き込む。シグネチャがそれを表しています。

## 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `cpp04_reference_const` — 参照と const で受け取る

```bash
./drill run cpp04
```

課題側からは `./drill read` でこの章に戻ってこられます。

## 参考

- [ROS 2 のコーディング規約](../ros2-コーディング規約.md) — ポインタは `char * c` と書く（`char* c` ではない）
- [rclcpp の設計思想](../rclcpp-の設計思想.md) 6章 — 購読コールバックの引数型とゼロコピーの関係
- `cppreference` の [Reference declaration](https://en.cppreference.com/w/cpp/language/reference) と [const qualifier](https://en.cppreference.com/w/cpp/language/cv)

---

前章 → [3. 継承](03_継承.md)
次章 → [5. ムーブと所有権](05_ムーブと所有権.md)
