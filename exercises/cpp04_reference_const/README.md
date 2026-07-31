# c04 参照と const で受け取る 〔C++編〕

const 参照と const メンバ関数を学びます。

## やること

`src/copycounter.cpp` に 2 つの関数を実装してください：

1. **copy_and_uppercase()**
   - const 参照で文字列を受け取る
   - テキストをすべて大文字に変換
   - 戻り値で新しい文字列を返す

2. **get_description()**
   - const メンバ関数として実装
   - 「コピー回数: X」の形式で説明文を返す

## 動かしてみる

```bash
./drill run c04
```

## つまずきポイント

- const 参照 `const std::string&` で受け取ると、コピーが発生しません。
- const メンバ関数は `int foo() const { ... }` の形です。
- const メンバ関数内では、メンバ変数を変更できません。

## テスト

```bash
./drill run c04
```

| テスト | 見ているところ |
| --- | --- |
| `constReferencePolicyで大文字に変換` | const 参照での効率的な受け取り |
| `constMemberFunctionが説明を返す` | const メンバ関数の構文 |

## 参考

- [cppreference: const qualifier](https://en.cppreference.com/w/cpp/language/const)
- [cppreference: Reference](https://en.cppreference.com/w/cpp/language/reference)
- [4. 参照と const で受け取る](../../docs/cpp/04_参照とconst.md)
