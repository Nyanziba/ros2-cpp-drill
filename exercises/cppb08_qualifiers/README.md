# cppb08 inline / explicit / constexpr / 末尾 const 〔C++入門編〕

宣言に付く修飾子を、4 つまとめて手で付ける課題です。

## この課題だけ編集対象がヘッダです

編集するのは `include/drill/qualifiers.hpp` です。`src/` ではありません。

`inline` `explicit` `constexpr` 末尾 `const` は、**どれも宣言に付ける修飾子**です。
`.cpp` 側の定義に後から付けることはできません。

```cpp
// これは通りません
explicit Pair::Pair(int a, int b) { }   // error: ‘explicit’ outside class declaration
```

だからこの 4 つを練習する課題では、編集対象がヘッダになります。

## やること

`include/drill/qualifiers.hpp` の TODO(1)〜(4) を埋めてください。

| TODO | 付ける修飾子 | 付けないとどうなるか |
| --- | --- | --- |
| (1) | `explicit` | `Meters m = 3.0;` が通ってしまう |
| (2) | 末尾 `const` | `const Meters` から `value()` を呼べない |
| (3) | `constexpr` | `static_assert(square(5) == 25)` がコンパイルできない |
| (4) | `inline` | 2 つの翻訳単位に実体ができて `multiple definition` |

## 動かしてみる

```bash
./drill run cppb08
```

**未着手のうちはビルドが通りません。** この課題で扱う 4 つはすべてコンパイル時の
性質なので、「テストが赤くなる」ではなく「コンパイルとリンクが止まる」形で出ます。
出るメッセージを 1 つずつ読んで、どの TODO の話なのかを対応させてください。

| メッセージ | 対応する TODO |
| --- | --- |
| `static assertion failed: Meters のコンストラクタに explicit を…` | (1) |
| `passing ‘const Meters’ as ‘this’ argument discards qualifiers` | (2) |
| `non-constant condition for static assertion` | (3) |
| `multiple definition of ‘twice(int)’` | (4) |

(4) はコンパイルではなくリンクで出ます。他の 3 つを直してからでないと見えません。

## つまずきポイント

- `explicit` は**引数が 1 つのコンストラクタで効きます**。`Meters(double)` のような
  1 引数コンストラクタは、付けないと暗黙の型変換の経路になります
- `constexpr` は `inline` を含みます。だから (3) を直せば `square` は多重定義になりません。
  `twice` だけ `inline` が必要なのはそのためです
- `constexpr` にしても実行時に呼べます。「コンパイル時**にも**評価できる」という意味です

## テスト

| テスト | 見ているところ |
| --- | --- |
| `Explicitが暗黙変換を止める` | `std::is_convertible_v<double, Meters>` が false か |
| `Const関数はConstオブジェクトから呼べる` | 末尾 `const` |
| `Constexprはコンパイル時に評価される` | `static_assert` が通るか |
| `Inlineで多重定義を避ける` | 2 つの翻訳単位からリンクできるか |

## 参考

- [8. その他の修飾子](../../docs/cpp-basics/08_その他の修飾子.md)
