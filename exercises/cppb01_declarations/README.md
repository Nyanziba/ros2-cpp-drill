# cppb01 宣言を読む 〔C++入門編〕

const とポインタの組み合わせを理解します。

## やること

`include/drill/reader.hpp` と `src/reader.cpp` に宣言と定義を書いてください。

関数：
- `int read_value(const int * p)`: const int へのポインタから値を読む
- `void modify_value(int * p, int new_val)`: int へのポインタ経由で値を修正
- `const int * get_constant_ptr(const int * p)`: const int へのポインタを返す

## 動かしてみる

```bash
./drill run cppb01
```

## つまずきポイント

- 宣言は右から左に読みます。`const int *` は「const int へのポインタ」です。
- ポインタの先を読み書きするときは逆参照 `*p` を使います。

## テスト

```bash
./drill run cppb01
```

| テスト | 見ているところ |
| --- | --- |
| `ConstPtrは読み取り専用` | const int * の読み出し |
| `非ConstPtrは変更可能` | int * への書き込み |
| `戻り値もConstPtrで正しい` | const int * の戻り値 |

## 参考

- [1. 宣言を読む](../../docs/cpp-basics/01_宣言を読む.md)
