# c01 ビルドとリンクを通す 〔C++編〕

**この課題だけ、いきなりビルドが落ちます。それが 1 問目です。**

C++ のビルドは「コンパイル」と「リンク」の 2 段階です。
この課題では両方に 1 つずつ用事があります。

## やること

編集するファイルは **2 つ**です。

| ファイル | 直すこと |
| --- | --- |
| `include/drill/counter.hpp` | リンクエラー（`multiple definition`）を消す |
| `src/counter.cpp` | `next_id()` を実装する |

### TODO(1) — リンクエラーを消す

まず `./drill run c01` を実行してください。テストは 1 つも走らず、こう落ちます。

```
multiple definition of `add_one(int)'
```

**ソースに `add_one` は 1 回しか書いていないのに「定義が複数ある」と言われます。**
なぜそうなるのかを考えてから、ヘッダのコメントを読んでください。

直し方は 2 通りあり、どちらでも合格します。

- `add_one` の定義に `inline` を付ける
- 定義を `src/counter.cpp` に移し、ヘッダには宣言だけ残す

### TODO(2) — `next_id()` を実装する

| 項目 | 値 |
| --- | --- |
| 機能 | 呼び出すたびに 1, 2, 3, ... と増える値を返す |
| 返り値の型 | `int` |
| 初回呼び出し | `1` |
| 2 回目以降 | 前回より 1 大きい値 |

ヒント: **`static` ローカル変数**を使います。

## 動かしてみる

```bash
./drill run c01
```

TODO(1) を直すまで、テストは 1 つも走りません。
**リンクが通らなければ実行ファイルができないので、テストする対象が存在しない**からです。
これが「コンパイルエラー」と「リンクエラー」の違いを体で覚える課題です。

シンボルを直接見ることもできます。

```bash
nm -C build/drill_cpp01_build_and_link/CMakeFiles/drill_cpp01_build_and_link_lib.dir/src/counter.cpp.o | grep add_one
```

`inline` を付ける前は `T add_one(int)`、付けた後は `W add_one(int)` になります。
`T` は「これが唯一の定義」、`W`（weak symbol）は「他にも同じものがあってよい」という意味です。

## つまずきポイント

- **`#pragma once` があるのに重複する。** `#pragma once` は「同じ翻訳単位に 2 回貼り付けない」
  ことしか保証しません。`.cpp` が 2 つあれば、翻訳単位も 2 つあります。
- **`inline` は「速くするため」ではありません。** 展開するかどうかは最適化器が勝手に決めます。
  現代の `inline` の実質的な意味は、この「定義が複数あってよい」という緩和です。
- **`static` ローカル変数の初期化は最初の 1 回だけです。** 2 回目以降は前回の値が残っています。
- `static` を関数の外に置くと「このファイルからしか見えない変数」という別の意味になります。
  スコープが狭いほうが安全なので、関数の中に置いてください。
- 返すのは `++id`（増やしてから返す）です。`id++` にすると初回に 0 が返ります。

## テスト

```bash
./drill run c01
```

| テスト | 見ているところ |
| --- | --- |
| `add_oneが1増やす` | TODO(1) を直してビルドが通ったか |
| `next_idが順番に増える` | `static` ローカル変数が値を保持しているか |

## 参考

- [1. ビルドとリンクの仕組み](../../docs/cpp/01_ビルドとリンクの仕組み.md) — 1.4 節が ODR、1.5 節がリンクエラーの読み方
- [cppreference: Storage duration](https://en.cppreference.com/w/cpp/language/storage_duration)
- [cppreference: inline specifier](https://en.cppreference.com/w/cpp/language/inline)
