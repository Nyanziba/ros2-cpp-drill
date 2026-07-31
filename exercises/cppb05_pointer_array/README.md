# cppb05 配列とポインタ 〔C++入門編〕

配列の decay と個数の取り扱いを学びます。

## やること

`src/array_util.cpp` に sum() と find_first() を実装してください。

- `sum()`: 配列の全要素の合計を返す
- `find_first()`: target に等しい最初の要素へのポインタを返す（見つからなければ nullptr）

## 動かしてみる

```bash
./drill run cppb05
```

## つまずきポイント

- 配列は関数引数でポインタに decay します。個数を別途渡す必要があります。
- `std::size_t` はプラットフォーム依存の unsigned 型です。符号付き int と比較するとき注意。

## テスト

```bash
./drill run cppb05
```

| テスト | 見ているところ |
| --- | --- |
| `Sum` | 配列のループ |
| `Sum空の配列` | count==0 の処理 |
| `FindFirst見つかる` | ポインタ返却 |
| `FindFirst見つからない` | nullptr 返却 |

## 参考

- [5. ポインタ2](../../docs/cpp-basics/05_ポインタ2.md)
