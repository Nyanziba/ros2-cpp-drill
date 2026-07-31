# c05 所有権をムーブする 〔C++編〕

ムーブセマンティクスを学びます。

## やること

`src/buffer.cpp` にムーブコンストラクタとムーブ代入を実装してください。

Buffer クラスは動的配列を管理します。コピーは禁止（deleted）で、ムーブのみ許可されています。

実装する関数：
- ムーブコンストラクタ: `Buffer(Buffer&& other) noexcept`
- ムーブ代入: `Buffer& operator=(Buffer&& other) noexcept`

## 動かしてみる

```bash
./drill run c05
```

## つまずきポイント

- ムーブ後、もとのオブジェクトは「空」状態になります。
  data_ は nullptr、size_ は 0 に設定します。
- ムーブ代入では、既存のメモリを解放してからムーブしてきます。
- 自己代入チェック `if (this != &other)` も忘れずに。

## テスト

```bash
./drill run c05
```

| テスト | 見ているところ |
| --- | --- |
| `ムーブコンストラクタがデータを転送する` | ムーブコンストラクタでのポインタ転送 |
| `ムーブ代入がデータを転送する` | ムーブ代入での正確な実装 |

## 参考

- [cppreference: Move semantics](https://en.cppreference.com/w/cpp/language/move)
- [cppreference: rvalue reference](https://en.cppreference.com/w/cpp/language/rvalue_reference)
- [5. 所有権をムーブする](../../docs/cpp/05_ムーブと所有権.md)
