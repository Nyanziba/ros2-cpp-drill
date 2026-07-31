# 7. `static`

> **この章のねらい**: `static` はポインタと同じく、**同じキーワードが 3 つの全く異なる意味で使われます。** どの場所に `static` があるかで意味が変わります。「スコープ内に見えない」「ファイル内にだけ見える」「クラスのインスタンス間で共有」の 3 つを見分ければ、`static` は予測可能になります。

## 7.1 関数内の `static` — 初回だけ初期化、呼び出しを跨いで生存

**関数の中の `static` 変数は、初回の関数呼び出しのときだけ初期化され、その後は値を保ったまま生存する。**

```cpp
#include <iostream>

int get_request_id()
{
  static int counter = 1000;
  return ++counter;
}

int main()
{
  std::cout << get_request_id() << "\n";  // 1001
  std::cout << get_request_id() << "\n";  // 1002
  std::cout << get_request_id() << "\n";  // 1003
}
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/oqaK3d54x)

```
1001
1002
1003
```

**初期化は最初の 1 回だけ。** 以降の呼び出しでは初期化をスキップして、前回の値から始まります。

これは**関数ローカル状態**を保つのに使われます。カウンター、ID 生成、キャッシュなど。
グローバル変数よりも `static` 関数ローカル変数の方が、**スコープが狭くて安全**です。

**C++11 以降、初期化はスレッドセーフ。**
複数スレッドから同じ関数を呼んでも、初期化は 1 回だけ起きます。

## 7.2 ファイル内の `static` — 外部リンケージを内部リンケージに

**ファイルの最上位（グローバル）で `static` を付けた変数は、その翻訳単位（`.cpp`）の中だけで見える。**

```cpp
// file1.cpp
static int counter = 0;

int get_counter_1()
{
  return ++counter;
}
```

```cpp
// file2.cpp
static int counter = 0;  // file1 の counter と別の変数

int get_counter_2()
{
  return ++counter;
}
```

`static` なしで同じ名前の変数を複数ファイルに書くと **link error** になります。

```cpp
// file1.cpp
int counter = 0;  // 外部リンケージ
```

```cpp
// file2.cpp
int counter = 0;  // file1 と重複 → multiple definition エラー
```

```
/usr/bin/ld: multiple definition of `counter'
```

**`static` で「このファイルだけで使う」と宣言すれば、リンク時に名前が衝突しない。**

同じ名前を複数ファイルで使うことができます。ただし、各ファイルに独立したコピーが存在します。
値を同期したければ、関数を通じて通信する必要があります。

## 7.3 クラスの `static` — インスタンス間で共有される状態

**`static` メンバは、そのクラスの全インスタンスで共有される単一のオブジェクト。**

```cpp
#include <iostream>

class Counter
{
public:
  Counter() { ++count_; }
  
  static int get_total() { return count_; }
  
private:
  static int count_;
};

int Counter::count_ = 0;  // 定義が必要

int main()
{
  std::cout << Counter::get_total() << "\n";  // 0
  
  Counter c1;
  std::cout << Counter::get_total() << "\n";  // 1
  
  Counter c2;
  std::cout << Counter::get_total() << "\n";  // 2
}
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/a19n4hYfW)

```
0
1
2
```

**`static` メンバは定義が必要。** 宣言だけではリンク時に見つかりません。

```
/usr/bin/ld: undefined reference to `Counter::count_'
```

**C++17 以降、`inline static` で定義不要になりました。**

```cpp
class Counter
{
private:
  inline static int count_ = 0;  // これで完全
};
```

### `static` メンバ関数 — `this` がない

メンバ関数に `static` を付けると、`this` を受け取らなくなります。

```cpp
class Logger
{
public:
  static void print_stats()  // this がない
  {
    std::cout << "instances: " << instance_count_ << "\n";
  }
  
private:
  inline static int instance_count_ = 0;
};

// クラス名で呼び出す
Logger::print_stats();
```

**`static` メンバ関数は、`static` メンバにしかアクセスできません。**
`instance_count_` は `static` だから OK。インスタンス固有のメンバは使えません。

## 7.4 `static` の 3 つの意味をまとめる

| 位置 | 例 | 生存期間 | スコープ | 初期化 |
| --- | --- | --- | --- | --- |
| 関数内 | `static int x = 0;` | プログラム全体 | その関数内だけ | 初回呼び出し時 1 回 |
| ファイル最上位 | `static int x = 0;` | プログラム全体 | そのファイル内だけ | プログラム開始時 |
| クラスメンバ | `static int x_;` | プログラム全体 | `ClassName::x_` で全体から | プログラム開始時 |

**どれが `static` か見分けるコツ：**

- `{}` の外で定義されているか、内か？ → 外なら「ファイル static」か「クラス static」
- `class` の中か？ → 中なら「クラス static」
- `class` の外の関数中か？ → 関数内なら「関数ローカル static」

## 手元で試す

関数内の `static`、ファイル内の `static`、クラスの `static` の 3 つを確認します。

```cpp
// static_all.cpp
#include <iostream>

class Logger
{
public:
  Logger(const char * name) : name_(name) { ++instance_count_; }
  
  void log(const char * msg) const
  {
    std::cout << name_ << ": " << msg << "\n";
  }
  
  static int instance_count() { return instance_count_; }
  
  static void print_stats()
  {
    std::cout << "Total instances created: " << instance_count_ << "\n";
  }
  
private:
  const char * name_;
  inline static int instance_count_ = 0;
};

int get_request_id()
{
  static int id = 1000;
  return ++id;
}

int main()
{
  std::cout << "== static in function ==\n";
  std::cout << "request 1: " << get_request_id() << "\n";
  std::cout << "request 2: " << get_request_id() << "\n";
  std::cout << "request 3: " << get_request_id() << "\n";
  
  std::cout << "\n== static in class ==\n";
  Logger log1("app");
  std::cout << "instances: " << Logger::instance_count() << "\n";
  
  Logger log2("system");
  std::cout << "instances: " << Logger::instance_count() << "\n";
  
  Logger::print_stats();
  
  log1.log("hello");
  log2.log("world");
  
  return 0;
}
```

**予想: `request_id` は 1001, 1002, 1003 になるか。`instance_count` は段階的に増えるか。**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic static_all.cpp -o static_all && ./static_all
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/dPocTz5e1)

```
== static in function ==
request 1: 1001
request 2: 1002
request 3: 1003

== static in class ==
instances: 1
instances: 2
Total instances created: 2
app: hello
system: world
```

3 点確認してください。

1. `get_request_id()` の呼び出しごとに `id` が増えている（初期化は 1 回だけ）
2. `Logger` のインスタンスを作るたびに `instance_count_` が増える
3. `Logger::print_stats()` はクラス名で呼び出せ、`static` メンバにアクセスできる

次に以下を試してください。

- `Logger` コンストラクタの `++instance_count_;` を非 static メンバに変えてコンパイルエラーを見る
- `inline static` を外して、`.cpp` ファイルでの定義を試す

## つまずきポイント

**`error: undefined reference to 'Counter::count_'`**
`static` メンバの定義を書き忘れています。クラス外で定義してください。
C++17 を使っているなら `inline static` にして、定義不要にすることもできます。

**`error: multiple definition of 'counter'`**
複数の `.cpp` ファイルで同じ名前のグローバル変数を定義しています。
どれか 1 つの定義を残して、他は `static` を付けるか、別ファイルに移してください。

**`static` メンバを関数から初期化できない**
初期化子リストまたはクラス定義内で初期化してください。

**クラスの `static` メンバにインスタンスからアクセスできる**
技術的には動きますが、`ClassName::member` で呼ぶべきです。意図が明確になります。

**関数ローカル `static` をスレッドセーフにしたい**
C++11 以降なら大丈夫。初期化は確実に 1 回だけです（Magic Statics）。

## 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `cppb07_static` — static の3つの意味

```bash
./drill run cppb07
```

詰まったら `./drill hint cppb07`、課題側からは `./drill read cppb07` でこの章に戻ってこられます。

## 参考

- `cppreference` の [Static storage duration](https://en.cppreference.com/w/cpp/language/storage_duration) と [Static members](https://en.cppreference.com/w/cpp/language/static)
- [ROS 2 のコーディング規約](../ros2-コーディング規約.md) — グローバル状態の使い方

---

前章 → [6. `const`](06_const.md)
次章 → [8. その他の修飾子](08_その他の修飾子.md)
