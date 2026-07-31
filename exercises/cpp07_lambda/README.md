# c07 ラムダとコールバックを登録する 〔C++編〕

ラムダ式と std::function を学びます。

## やること

`src/callbackmanager.cpp` に CallbackManager クラスを実装してください。

CallbackManager はコールバック関数を登録・実行するクラスです。

実装する関数：
- `register_callback()`: std::function をリストに追加
- `fire()`: 登録されたすべてのコールバックを呼び出す

## 動かしてみる

```bash
./drill run c07
```

## つまずきポイント

- `std::function<void(int)>` は任意の void(int) コールバックを保持できます。
- ラムダ式 `[&x](int v) { ... }` で値や参照をキャプチャできます。

## テスト

```bash
./drill run c07
```

| テスト | 見ているところ |
| --- | --- |
| `コールバックが呼び出される` | register_callback と fire の動作 |
| `複数のコールバックが登録できる` | 複数コールバックの管理 |

## 参考

- [cppreference: Lambda expressions](https://en.cppreference.com/w/cpp/language/lambda)
- [cppreference: std::function](https://en.cppreference.com/w/cpp/utility/functional/function)
- [7. ラムダとコールバック](../../docs/cpp/07_ラムダとstd_bind.md)
