# cppb04 ポインタを使う 〔C++入門編〕

nullptr チェックとポインタの逆参照を学びます。

## やること

`src/checker.cpp` に try_read() を実装してください。

- 両方のポインタが nullptr でなければ、読み込みポインタの先の値を書き込みポインタの先に書く
- どちらかが nullptr なら false を返す

## 動かしてみる

```bash
./drill run cppb04
```

## つまずきポイント

- `nullptr` チェックはセグメンテーション違反を防ぎます。
- 逆参照 `*p` は nullptr でないことが前提です。

## テスト

```bash
./drill run cppb04
```

| テスト | 見ているところ |
| --- | --- |
| `両方がvalidな場合` | 正常系 |
| `読み込みポインタがnullptr` | nullptr チェック |
| `書き込みポインタがnullptr` | nullptr チェック |
| `両方がnullptr` | 両方チェック |

## 参考

- [4. ポインタ1](../../docs/cpp-basics/04_ポインタ1.md)
