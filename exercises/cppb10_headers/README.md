# cppb10 ヘッダに分ける 〔C++入門編〕

宣言と定義を分けて複数の .cpp をリンクさせる課題です。

## やること

`src/math.cpp` に add() を、`src/util.cpp` に multiply() を実装してください。

- `include/drill/math.hpp`：宣言のみ（編集しない）
- `src/math.cpp`：add() の定義
- `src/util.cpp`：multiply() の定義
- テスト：両方を使う

## 動かしてみる

```bash
./drill run cppb10
```

## つまずきポイント

- 複数の `.cpp` ファイルが同じヘッダを `#include` しても OK（宣言だから）。
- ビルド時に 2 つの目的ファイルをリンクして 1 つの実行ファイルを作ります。
- include guard（`#pragma once`）を忘れずに。

## テスト

```bash
./drill run cppb10
```

| テスト | 見ているところ |
| --- | --- |
| `Add` | math.cpp |
| `Multiply` | util.cpp |
| `両方を一緒に使う` | リンク成功 |

## 参考

- [10. ヘッダとプロジェクト構成](../../docs/cpp-basics/10_ヘッダとプロジェクト構成.md)
