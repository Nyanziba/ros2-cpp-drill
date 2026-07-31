# c10 時間を型で扱う 〔C++編〕

std::chrono を学びます。

## やること

`src/timer.cpp` に 2 つの関数を実装してください：

1. **count_ticks()**
   - milliseconds の予算からティック数を計算
   - `duration_cast` を使う

2. **seconds_to_ms()**
   - 秒（double）を std::chrono::milliseconds に変換

## 動かしてみる

```bash
./drill run c10
```

## つまずきポイント

- `std::chrono::duration_cast<T>(d)` で型を変換
- `duration.count()` で数値を取得
- 時間計算の精度に注意

## テスト

```bash
./drill run c10
```

## 参考

- [cppreference: std::chrono](https://en.cppreference.com/w/cpp/chrono)
- [cppreference: duration_cast](https://en.cppreference.com/w/cpp/chrono/duration/duration_cast)
- [10. 時間を型で扱う](../../docs/cpp/12_chronoと時間.md)
