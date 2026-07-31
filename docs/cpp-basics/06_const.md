# 6. `const`

> **この章のねらい**: `const` は宣言の 4 か所に出てきます。各位置で意味が異なり、見分け方が重要です。**4 つの場所ごとに異なるルールが有効**であることを理解することで、`const` がつくコードが確実に読めるようになります。この章は宣言を読む力を大きく伸ばします。

## 6.1 `const` な値の初期化と代入

**値そのものを `const` にする。**

```cpp
const int x = 5;
```

右側に置く読み方は「`int` 定数 `x`」。型の右側に `const` を置くのは「そのもの」が `const` という意味です（章 1 参照）。

**初期化は必須。一度決めたら変えられない。**

```cpp
const int x = 5;
x = 10;  // エラー
```

```
error: assignment of read-only variable 'x'
```

`const` な値は「変わらないことが保証される」ので、コンパイラが最適化しやすくなります。
また、ユーザーが「これは変わらない」という意思を示せるので、コードの意図が読みやすくなります。

## 6.2 関数パラメータの `const`

**関数の引数として `const` 参照を受け取り、呼び出し側のオブジェクトを保護する。**

```cpp
void process(const std::string & s)
{
  // s は参照だが const なので書き換えられない
}
```

このパターンが **最頻出**です。`std::string` や大きなオブジェクトをパラメータにするときは、
**コピーを避けるために `const &` を使う**。その結果として、意図しない変更が防がれます。

```cpp
void process(const std::string & s)
{
  s = "modified";  // エラー
}
```

```
error: no match for ‘operator=’ (operand types are ‘const std::string’ and ‘const char [9]’)
```

**メッセージは型によって変わります。** ここは `std::string` なので
「`const std::string` に代入する `operator=` が無い」という形で出ます。
`const int &` のようなスカラーだと、もっと素直な文になります。

```cpp
void bump(const int & n)
{
  n = 1;   // エラー
}
```

```
error: assignment of read-only reference ‘n’
```

**言い回しが違っても意味は同じです。**「`const` を通しては書けない」と言われています。
`read-only` という単語を目印にしてください。

このエラーは **関数を書く側が立てた約束**です。
「このオブジェクトは読むだけで書き換えない」とシグネチャで宣言しているので、
呼び出し側は中身を読まずに「渡しても壊されない」と判断できます。
コピーを避けつつ安全、というのが `const &` の値打ちです。

## 6.3 メンバ関数の末尾の `const` — 最重要

**メンバ関数の `)` の直後に `const` を置くと「このメンバ関数は `*this` を書き換えない」という約束になる。**

```cpp
class Point
{
public:
  Point(double x, double y) : x_(x), y_(y) {}
  
  double distance() const  // ← メンバ関数の const
  {
    return std::sqrt(x_ * x_ + y_ * y_);
  }
  
  void move(double dx, double dy)  // ← const なし = 書き換え可
  {
    x_ += dx;
    y_ += dy;
  }
  
private:
  double x_, y_;
};
```

**`const` メンバ関数は他の `const` メンバ関数だけを呼べる。**

```cpp
class Point
{
public:
  double distance() const
  {
    return distance_squared();  // OK（const 呼び出し）
  }
  
  double distance_squared() const
  {
    return x_ * x_ + y_ * y_;
  }
  
  void move(double dx, double dy)
  {
    // distance_squared();  // これは OK
    x_ += dx;
  }
  
private:
  double x_, y_;
};
```

**`const` なオブジェクトは `const` メンバ関数だけを呼べる。**

```cpp
const Point p(3.0, 4.0);
std::cout << p.distance() << "\n";  // OK
// p.move(1.0, 1.0);  // エラー - const メンバ関数ではない
```

エラーメッセージ：

```
error: passing 'const Point' as 'this' argument discards qualifiers [-fpermissive]
```

このルール（**`const` 性の分離**）が非常に重要です。
読み専用のメンバ関数と、書き込みのメンバ関数が明確に区別されます。

## 6.4 ポインタの `const` — 左結合の話

ポインタに `const` が付く場合、位置で意味が変わります（章 1 の左結合を思い出してください）。

**`const int * p` — 指す先が `const`**

```cpp
const int * p = &a;
*p = 10;  // エラー
p = &b;   // OK - ポインタは動かせる
```

「`int` の `const` ポインタ」ではなく「`const int` へのポインタ」と読みます。

**`int * const p` — ポインタそのものが `const`**

```cpp
int * const p = &a;
*p = 10;  // OK - 指す先は変えられる
p = &b;   // エラー
```

「`const` な `int` ポインタ」と読みます。

**覚え方：`const` の左側を読む。**

- `const int *` — `const` の左は `int`、つまり `int` が `const`
- `int * const` — `const` の左は `*`、つまりポインタが `const`

表で整理します。

| コード | 意味 | 指す先を変更 | ポインタを変更 |
| --- | --- | --- | --- |
| `const int * p` | pointee が const | ✗ | ○ |
| `int * const p` | pointer が const | ○ | ✗ |
| `const int * const p` | 両方 const | ✗ | ✗ |

## 6.5 `const` の 4 つの位置まとめ

| 位置 | 例 | 意味 | 何が const か |
| --- | --- | --- | --- |
| 宣言 | `const int x = 5;` | 値が定数 | その変数 |
| 参照パラメータ | `void f(const T & x)` | 読み専用を約束 | パラメータの参照先 |
| メンバ関数末尾 | `void f() const` | `*this` は読み専用 | メンバ関数内の状態 |
| ポインタ | `const int * p` / `int * const p` | 指す先 / ポインタ自身 | 位置で決まる |

## 6.6 `mutable` — `const` メンバ関数から書き込みを許す

`const` メンバ関数内でメンバを書き換えたい場合がある。
**キャッシュ**や**アクセスカウンタ**のように、「読み側では見えない内部状態」の更新です。

```cpp
class Cached
{
public:
  Cached(double x, double y) : x_(x), y_(y), cached_(false) {}
  
  double distance() const
  {
    if (!cached_) {
      cached_ = true;           // mutable なので書き込み OK
      distance_ = std::sqrt(x_ * x_ + y_ * y_);
    }
    return distance_;
  }
  
private:
  double x_, y_;
  mutable bool cached_;         // ← この 2 つは const メンバ関数から書き込み可
  mutable double distance_;
};
```

**`mutable` は最後の手段。** むやみに使うと「読み専用」という約束を破ることになります。
本当にキャッシュや同期フラグなど、外から見えない内部状態だけに使ってください。

## 手元で試す

`const` の 4 つの位置と、`const` オブジェクトの制約を確認します。

```cpp
// const_all.cpp
#include <iostream>
#include <string>

class Rectangle
{
public:
  Rectangle(double w, double h) : width_(w), height_(h), access_count_(0) {}
  
  double area() const
  {
    ++access_count_;  // mutable なのでOK
    return width_ * height_;
  }
  
  void resize(double w, double h)
  {
    width_ = w;
    height_ = h;
  }
  
  int access_count() const { return access_count_; }
  
private:
  double width_, height_;
  mutable int access_count_;
};

void print_area(const Rectangle & rect)
{
  std::cout << "area: " << rect.area() << "\n";
}

int main()
{
  std::cout << "== const オブジェクト ==\n";
  const Rectangle r(3.0, 4.0);
  std::cout << "r.area() = " << r.area() << "\n";
  std::cout << "access_count: " << r.access_count() << "\n";
  
  std::cout << "\n== const 参照パラメータ ==\n";
  print_area(r);
  
  std::cout << "\n== const ポインタ ==\n";
  int a = 10, b = 20;
  const int * p1 = &a;
  std::cout << "pointee is const: *p1 = " << *p1 << ", can move pointer\n";
  p1 = &b;
  std::cout << "now p1 points to b: *p1 = " << *p1 << "\n";
  
  int * const p2 = &a;
  std::cout << "pointer is const: *p2 = " << *p2 << ", can modify pointee\n";
  *p2 = 99;
  std::cout << "after *p2 = 99: a = " << a << "\n";
  
  return 0;
}
```

**予想: `access_count` の出力は何か。`p1` と `p2` でどちらが動かせるか。`a` は 99 になるか。**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic const_all.cpp -o const_all && ./const_all
```

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/xGYxjzo8T)

```
== const オブジェクト ==
r.area() = 12
access_count: 1

== const 参照パラメータ ==
area: 12

== const ポインタ ==
pointee is const: *p1 = 10, can move pointer
now p1 points to b: *p1 = 20
pointer is const: *p2 = 10, can modify pointee
after *p2 = 99: a = 99
```

3 点確認してください。

1. `mutable` のおかげで `access_count` は 1 回目で 1 になっている（`const` メンバ関数内の更新が成功）
2. `p1` はポインタが `const` ではないので `&b` に動かせる
3. `p2` はポインタが `const` だが、指す先は `const` ではないので `99` に変更できる

次に、以下の行を 1 つずつコメント外してエラーメッセージを読んでください。

- `r.resize(5.0, 5.0);` — const オブジェクトは非 const メンバ関数を呼べない
- `*p1 = 30;` — const int ポインタで指す先を変更しようとしている
- `p2 = &b;` — const ポインタを再代入しようとしている

## つまずきポイント

**`error: assignment of read-only variable 'x'`**
`const` な変数に代入しようとしています。初期化のとき値を決めてください。

**`error: assignment of read-only reference 'n'`** / **`error: no match for ‘operator=’`**
`const` 参照パラメータを書き換えようとしています。
意図が「読み専用」なら参照先を書き換えないでください。
意図が「書き換えたい」なら `const` を外してください。

**`error: passing 'const Point' as 'this' argument discards qualifiers`**
`const` オブジェクトから非 `const` メンバ関数を呼んでいます。
メンバ関数に `const` を付けて、「このメンバ関数は読み専用」にしてください。

**`error: assignment of member '...' in read-only object`**
`const` メンバ関数の中からメンバを書き換えようとしています。
メンバが `mutable` でない限り、`const` メンバ関数からは書き込みはできません。

**`const int * const p` の 2 つの `const` の見分けがつかない**
左結合で読んでください。左側の `const` は「指す先が const」。右側の `const` は「ポインタが const」。

## 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `cppb06_const` — const を宣言と定義で合わせる

```bash
./drill run cppb06
```

詰まったら `./drill hint cppb06`、課題側からは `./drill read cppb06` でこの章に戻ってこられます。

## 参考

- `cppreference` の [const-qualified type](https://en.cppreference.com/w/cpp/language/const_cast) と [Member functions](https://en.cppreference.com/w/cpp/language/member_functions)
- [ROS 2 のコーディング規約](../ros2-コーディング規約.md) — `const` な参照パラメータを「読み専用」として使う実装パターン

---

前章 → [5. ポインタ② — 配列と使い分け](05_ポインタ2.md)
次章 → [7. `static`](07_static.md)
