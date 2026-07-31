# c08 演算子をオーバーロードする 〔C++編〕

## やること

`src/vec2.cpp` に **8 つの演算子**を実装します。Vec2（2次元ベクトル）に対して、足し算・引き算・スカラー倍・比較・ストリーム出力を実装します。

| 演算子 | 実装する型 | 説明 |
| --- | --- | --- |
| `operator+` | 自由関数 | `a + b` で 2 つのベクトルを足す |
| `operator-` | 自由関数 | `a - b` で 2 つのベクトルを引く |
| `operator*` （左右両方） | 自由関数 | `v * s` と `s * v` の両方でスカラー倍 |
| `operator+=` | メンバ関数 | `a += b` で自分に足し込み、自身への参照を返す |
| `operator==` | 自由関数 | `a == b` で等価性を判定 |
| `operator!=` | 自由関数 | `a != b` で非等価性を判定（`operator==` を使って実装する） |
| `operator<` | 自由関数 | `a < b` で大小比較。`length_squared()` の値で比較 |
| `operator<<` | 自由関数 | `std::cout << v` で `(x, y)` の形で出力 |

**重要: 対称な演算子（`+`, `-`, `*`, `==`, `!=`, `<`）は自由関数として実装します。** メンバ関数では `2.0 * v` が書けません。左辺が `double` なので、`double` のメンバ関数として書く必要があり、それはできないからです。詳しくは [10. 演算子オーバーロード](../../docs/cpp/10_演算子オーバーロード.md) の 10.2 節。

## 動かしてみる

```bash
./drill run c08
```

すべての 8 つのテストが成功すると、課題クリアです。

## つまずきポイント

**`operator*` は 2 つ書かないといけない**

`Vec2 * double` と `double * Vec2` は別の関数です。

```cpp
Vec2 operator*(const Vec2 & v, double s) { ... }
Vec2 operator*(double s, const Vec2 & v) { ... }
```

メンバ関数として書くと、左辺が `double` の場合が書けません。テストの「スカラー倍は左右どちらの順番でも書ける」で両方が要求されます。

**`operator<` は `<=` ではなく `<` を使う**

`operator<` で「小さいまたは等しい」という意味で `<=` を使うと、テストの「大小比較は厳密弱順序である」で `std::sort` が未定義動作になります。理由は [10. 演算子オーバーロード](../../docs/cpp/10_演算子オーバーロード.md) の 10.3 節を参照。

```cpp
bool operator<(const Vec2 & a, const Vec2 & b) {
  return a.length_squared() < b.length_squared();  // <= ではなく <
}
```

**`operator<<` は `std::ostream &` を返さないと `<<` が繋げられない**

```cpp
std::ostream & operator<<(std::ostream & os, const Vec2 & v) {
  os << "(" << v.x << ", " << v.y << ")";
  return os;  // 絶対に書く。これがないと << が繋げられない
}
```

テストの「ostream演算子は繋げられる」で `oss << "a=" << Vec2{1.0, 2.0} << " b=" << Vec2{3.0, 4.0}` と書き続けているので、各 `<<` が `std::ostream &` を返す必要があります。

**`operator+=` は `Vec2 &` を返し、ムーブではなく `*this` を返す**

```cpp
Vec2 & Vec2::operator+=(const Vec2 & other) {
  x += other.x;
  y += other.y;
  return *this;  // &を付けて、アドレスが同じ参照を返す
}
```

コピーではなく参照を返すこと。テストの「加算代入は自分自身への参照を返す」で `EXPECT_EQ(&returned, &a)` とアドレスを比較して確認されます。

**`operator!=` は `operator==` を使って書く**

```cpp
bool operator!=(const Vec2 & a, const Vec2 & b) {
  return !(a == b);
}
```

`operator==` が正しく実装されていれば、`!=` は自動で正しくなります。2 箇所に同じ論理を書かないこと。

## テスト

```bash
./drill run c08
```

| テスト | 見ているところ |
| --- | --- |
| `足し算と引き算` | `operator+` と `operator-` の実装 |
| `スカラー倍は左右どちらの順番でも書ける` | `operator*` が左右両方（`v * 2.0` と `2.0 * v`）で動くか |
| `加算代入は自分自身への参照を返す` | `operator+=` が `Vec2 &` を返し、同じアドレスを返すか |
| `等価比較と非等価比較` | `operator==` と `operator!=` の実装、及びどちらの成分が違う場合も検出するか |
| `大小比較は厳密弱順序である` | `operator<` が `<` を使っており、`a < a` が false、長さが同じなら両方 false |
| `std_sortで並べられる` | `operator<` が正しく `std::sort` で使える |
| `ostreamに流せる` | `operator<<` が `(x, y)` の形で出力するか |
| `ostream演算子は繋げられる` | `operator<<` が `std::ostream &` を返し、`<<` が繋げられるか |

## 参考

- [10. 演算子オーバーロード](../../docs/cpp/10_演算子オーバーロード.md) — 10.2 節（メンバと自由関数の使い分け）、10.3 節（比較演算子）
- [cppreference: Operator overloading](https://en.cppreference.com/w/cpp/language/operators)
