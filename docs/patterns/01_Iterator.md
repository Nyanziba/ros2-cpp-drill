# 1. Iterator

> **結城本 第1章 対応。** `BookShelf` と `BookShelfIterator` を手元に開いてください。
>
> **この章のねらい**: Java 版の `Iterator` は `hasNext()` と `next()` の 2 つのメソッドです。
> C++ の `std::vector` のイテレータは `operator++` と `operator*` と `operator!=` です。
> **なぜ形が違うのか。** GoF 版を自分で実装してから、STL 版を実装して、
> 同じ `BookShelf` を両方の方法で走査します。
> 違いは好みではなく、**「終端をどう表すか」の設計判断**から来ています。

## 1.1 Java 版をそのまま C++ にすると

結城本の `Iterator` インタフェースはこうです。

```java
public interface Iterator {
    public abstract boolean hasNext();
    public abstract Object next();
}
```

C++ に素直に移すとこうなります。

```cpp
class Iterator
{
public:
  virtual ~Iterator() = default;
  virtual bool has_next() const = 0;
  virtual const Book & next() = 0;
};
```

Java 版から変えた点が 3 つあります。**どれも変えないとバグります。**

### 変更点1: `virtual ~Iterator() = default;` を足した

Java には要りませんが、C++ では**必須**です。

```cpp
std::unique_ptr<Iterator> it = shelf.iterator();
// it が破棄されるとき、Iterator のデストラクタしか呼ばれない
// → BookShelfIterator が持っていたメンバが解放されない
```

基底クラスのポインタで `delete` するのに、デストラクタが仮想でないと
**派生クラスのデストラクタが呼ばれません**。未定義動作です。

**ルール**: 純粋仮想関数を 1 つでも書いたら、仮想デストラクタも書く。
この講習の 23 章すべてで守ります。

### 変更点2: `Object` を `const Book &` にした

Java 版は `Object` を返してキャストで戻します。C++ にその文化はありません。
テンプレートか、具体的な型を返します。
**この章では具体型で書きます。** テンプレート化は 1.5 でやります。

`&`（参照）にしたのは、**`Book` のコピーを避けるため**です。
Java では参照が返るのが当たり前ですが、C++ で `Book next()` と書くと**毎回コピーが走ります**。

```cpp
virtual Book next();              // Book をコピーして返す
virtual const Book & next();      // 本棚の中の Book をそのまま指す。コピーなし
```

ここが Java との一番大きい差です。**書かないとコピーになる。**

### 変更点3: `has_next()` に `const` を付けた

`has_next()` は状態を変えないので `const` メンバ関数にします。
`next()` は位置を進めるので `const` を付けません。
**Java にはこの区別が無い**ので、書き忘れが起きます。

## 1.2 誰が Iterator を所有するのか

Java 版の `iterator()` はこうです。

```java
public Iterator iterator() {
    return new BookShelfIterator(this);
}
```

`new` して返すだけ。GC が回収します。C++ で同じことを書くと、

```cpp
Iterator * iterator() const;      // 呼んだ人が delete する？　書いてないと分からない
```

**誰が `delete` するかが型に書かれていません。** これは C++ では設計の欠陥です。
`unique_ptr` を返します。

```cpp
std::unique_ptr<Iterator> iterator() const;
```

これで「**受け取った人が所有する。スコープを抜ければ勝手に消える**」が型に書かれました。
呼ぶ側はこうなります。

```cpp
auto it = shelf.iterator();       // std::unique_ptr<Iterator>
while (it->has_next()) {
  std::cout << it->next().name() << "\n";
}
// ここで自動的に解放される
```

**23 章のうち多くで同じ判断をします**。「Java が `new` して返しているところは、
C++ では `std::unique_ptr` を返す」と覚えてください。

## 1.3 Java には無い危険 — イテレータが本棚より長生きする

C++ 固有の落とし穴です。

```cpp
std::unique_ptr<Iterator> make_iterator()
{
  BookShelf shelf;                 // ローカル変数
  shelf.append(Book{"Design Patterns"});
  return shelf.iterator();         // shelf はここで死ぬ
}                                  // 返ってきたイテレータは死んだ本棚を指している

auto it = make_iterator();
it->has_next();                    // 未定義動作
```

`BookShelfIterator` は `BookShelf` へのポインタか参照を持ちます。
**Java なら GC が `shelf` を生かしておくので落ちません。C++ では落ちます。**

これは Iterator パターンの欠陥ではなく、**C++ でオブジェクトへの参照を保持する型を作ると
必ず付いてくる制約**です。第 12 章（Decorator）、第 14 章（Chain of Responsibility）、
第 17 章（Observer）でも同じ問題が出ます。

**対処は 3 つ**あります。

| 方法 | 書き方 | いつ使うか |
| --- | --- | --- |
| 寿命を約束する（規約） | 「イテレータは本棚より長生きさせないこと」とコメント | STL がこれ。コストゼロ |
| `shared_ptr` で共有所有 | `shared_ptr<const BookShelf>` を持つ | 寿命が読めないとき |
| コピーを持つ | イテレータが `vector<Book>` をコピー | 小さいときだけ |

**STL は 1 番**を選んでいます。`std::vector::iterator` は vector が死ねば無効になります。
「イテレータの無効化」と呼ばれる、あの話です。
この課題でも 1 番で書きます。**コメントに書く**ことが仕事です。

## 1.4 STL 版 — なぜ `hasNext()` が無いのか

C++ の走査はこう書きます。

```cpp
for (auto it = shelf.begin(); it != shelf.end(); ++it) {
  std::cout << it->name() << "\n";
}
```

`hasNext()` はどこにもありません。代わりに `it != shelf.end()` があります。
**終端を「イテレータが答える」のではなく「終端を表すイテレータと比べる」設計**です。

この差が何を生むか。

| | GoF 版（`hasNext`） | STL 版（`begin` / `end`） |
| --- | --- | --- |
| 終端の表現 | イテレータ自身が知っている | 終端を表す別のイテレータ |
| 部分範囲 | 表現できない | `it2` から `it5` まで、が書ける |
| アルゴリズムの共通化 | できない | `std::find` `std::count_if` `std::sort` が全部使える |
| 仮想関数呼び出し | 毎回 2 回（`hasNext` と `next`） | ゼロ（インライン展開される） |

**3 行目が本命です。** `begin()` / `end()` を用意した瞬間に、
**`<algorithm>` の 100 個以上の関数が全部使えるようになります。**

```cpp
auto found = std::find_if(shelf.begin(), shelf.end(),
                          [](const Book & b) { return b.name() == "Refactoring"; });

int n = std::count_if(shelf.begin(), shelf.end(),
                      [](const Book & b) { return b.name().size() > 10; });
```

GoF 版の `Iterator` ではこれが 1 つも使えません。自分で while ループを書きます。

さらに、`begin()` / `end()` があると **range-based for** が動きます。

```cpp
for (const Book & book : shelf) {
  std::cout << book.name() << "\n";
}
```

コンパイラが `begin()` / `end()` を探して上のループに展開しているだけです。
**特別な機能ではありません。名前の規約です。**

### 4 行目 — 仮想関数のコスト

GoF 版は `has_next()` と `next()` が仮想関数なので、
1 要素につき**仮想関数呼び出しが 2 回**入ります。インライン展開もされません。

STL 版のイテレータは（この課題では）`std::vector<Book>::const_iterator` そのままなので、
**実質ポインタ**です。呼び出しコストはゼロです。

マイコンで数千要素を回すなら、この差は実測できます。

## 1.5 テンプレート化はしない（この章では）

「`Book` 決め打ちじゃなくてテンプレートにすべきでは」と思ったはずです。正しいですが、
**この章ではやりません。** 理由は 2 つです。

1. 結城本第1章の主題は「走査と中身を分離する」ことで、汎用化ではない
2. C++ のテンプレートでイテレータを正しく書くと、`iterator_traits` や
   `value_type` / `difference_type` の定義が要る。**そこは Iterator パターンの話ではない**

STL 互換のイテレータを 1 から書く方法は、必要になったときに
[C++編 9. テンプレートの読み方](../cpp/09_テンプレートの読み方.md) の先で扱います。
この課題では、**`std::vector` のイテレータをそのまま公開**します。
実務でもこれが正解であることが大半です。

## 1.6 手元で試す

課題を解く前に、この 1 ファイルをコンパイルして**出力を予想してから**実行してください。

```cpp
#include <iostream>
#include <string>
#include <vector>

class Book
{
public:
  explicit Book(std::string name) : name_(std::move(name)) {}
  const std::string & name() const { return name_; }

private:
  std::string name_;
};

class Shelf
{
public:
  void append(Book book) { books_.push_back(std::move(book)); }

  // これだけで range-based for も <algorithm> も動く
  std::vector<Book>::const_iterator begin() const { return books_.begin(); }
  std::vector<Book>::const_iterator end() const { return books_.end(); }

private:
  std::vector<Book> books_;
};

int main()
{
  Shelf shelf;
  shelf.append(Book{"Design Patterns"});
  shelf.append(Book{"Refactoring"});

  for (const Book & book : shelf) {
    std::cout << book.name() << "\n";
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 何行出るか。そして <code>begin()</code> / <code>end()</code> を消したらどうなるか</summary>

2 行出ます。

```
Design Patterns
Refactoring
```

`begin()` / `end()` を消すと、range-based for がコンパイルエラーになります。

```
error: 'begin' was not declared in this scope
```

**range-based for は `begin()` / `end()` という名前を探しているだけ**だと分かります。
継承もインタフェースも要りません。
</details>

## 1.7 マイコンでの結論

**GoF 版の `Iterator` は使いません。** 理由は 2 つです。

1. `iterator()` が `std::unique_ptr` を返す = **走査のたびにヒープ確保**が走る
2. 仮想関数呼び出しが 1 要素あたり 2 回

ループの中で `new` するコードは、マイコンでは書けません。断片化して、いずれ確保に失敗します。

代わりに `begin()` / `end()` を書きます。**確保はゼロ、仮想関数もゼロ**です。

```cpp
// 固定長。動的確保なし
template <std::size_t N>
class SensorBuffer
{
public:
  const int * begin() const { return data_; }
  const int * end() const { return data_ + size_; }

private:
  int data_[N] = {};
  std::size_t size_ = 0;
};
```

**生ポインタもイテレータです。** `++` と `*` と `!=` が使えれば、
`std::find` も range-based for も動きます。
`std::vector` も `std::unique_ptr` も要りません。

どうしても走査方法を実行時に切り替えたいなら、
`std::unique_ptr` ではなく**呼び出し側が用意したバッファに構築する**か、
テンプレートでコンパイル時に決めてください（第 10 章 Strategy で扱います）。

## 1.8 ROS 2 での結論（補足）

rclcpp にも GoF 版の `Iterator` クラスは出てきません。
必要なところは STL のイテレータか range-based for です。

```cpp
for (const auto & param : this->get_parameters(names)) { /* ... */ }
```

`sensor_msgs::msg::PointCloud2` を走査する `PointCloud2Iterator` は、
名前こそ Iterator ですが**中身は STL 互換のイテレータ**で、`operator++` と `operator*` を持ちます。
`hasNext()` ではありません。

## 1.9 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `it->name()` で毎回コピーされている | `next()` が `Book` を返している。`const Book &` にする |
| イテレータを解放したらプログラムが落ちる | `Iterator` の仮想デストラクタが無い |
| `error: 'begin' was not declared in this scope` | `begin()` / `end()` が `public` でない、または名前が違う |
| range-based for で書き換えようとしたらエラー | `const_iterator` を返している。書き換えるなら `iterator` 版も要る |
| 2 つのイテレータを同時に使うと位置が混ざる | イテレータが位置を**本棚側**に持っている。位置はイテレータが持つ |
| 本棚を返す関数からイテレータを返したら落ちた | 1.3 の寿命の問題 |

## 1.10 対応する課題

```bash
./drill run dp01
```

`exercises/dp01_iterator/src/book_shelf.cpp` に、

1. GoF 版の `BookShelfIterator`（`has_next()` / `next()`）
2. `BookShelf::iterator()` — `std::unique_ptr<Iterator>` を返す
3. `BookShelf::begin()` / `end()` — STL 版

を実装します。テストは**同じ本棚を両方の方法で走査して、結果が一致すること**と、
STL 版で `std::count_if` が動くことを見ます。

## 1.11 この章のまとめ

- Java の `interface` を C++ に移したら、**仮想デストラクタを足す**
- `Object` を返すところは具体型。**`&` を付けないとコピーになる**
- Java の `new` して返すところは **`std::unique_ptr` を返す**
- **イテレータは元のコンテナより長生きできない**。Java には無い制約
- GoF 版は「イテレータが終端を知る」、STL 版は「終端を表すイテレータと比べる」
- **`begin()` / `end()` を書くと `<algorithm>` と range-based for が丸ごと付いてくる**
- マイコンでは GoF 版は使わない。`unique_ptr` の確保と仮想関数呼び出しが乗るため

---

前: [0. 使う前に](00_使う前に.md) ／ 次: 2. Adapter（準備中）
