# c09 標準ライブラリを使う 〔C++編〕

std::clamp と std::optional を学びます。

## やること

`src/limiter.cpp` に 2 つの関数を実装してください：

1. **clamp_velocity()**
   - `std::clamp` を使って値を範囲内に限定

2. **find_user_id()**
   - `std::map` から値を探す
   - `std::optional` で結果を返す

## 動かしてみる

```bash
./drill run c09
```

## つまずきポイント

- `std::clamp(val, min, max)` で min～max の範囲に限定
- `std::optional<T>` は値を持つ場合と持たない場合を表現
- `std::nullopt` で「値がない」を表す

## テスト

```bash
./drill run c09
```

## 参考

- [cppreference: std::clamp](https://en.cppreference.com/w/cpp/algorithm/clamp)
- [cppreference: std::optional](https://en.cppreference.com/w/cpp/utility/optional)
- [9. 標準ライブラリ](../../docs/cpp/11_標準ライブラリの道具箱.md)
