# cppb03 参照で受け取る 〔C++入門編〕

参照を使った値の修正を学びます。

## やること

`src/swapper.cpp` に swap_values() と largest() を実装してください。

- `swap_values()`: 呼び出し元の 2 つの int を入れ替える
- `largest()`: より大きい方への参照を返す

## 動かしてみる

```bash
./drill run cppb03
```

## つまずきポイント

- 参照 `&` は別名です。参照を修正すれば呼び出し元も変わります。
- 参照を返すとき、そのオブジェクトがスコープを抜けないことを確認してください。

## テスト

```bash
./drill run cppb03
```

| テスト | 見ているところ |
| --- | --- |
| `SwapsTwoVariablesByReference` | 参照による変更 |
| `ReturnsReferenceSoCallerCanModify` | 参照の戻り値 |
| `ReturnsFirstWhenEqual` | エッジケース |

## 参考

- [3. 参照](../../docs/cpp-basics/03_参照.md)
