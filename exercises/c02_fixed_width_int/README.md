# c02: 固定幅整数と暗黙変換

C言語の固定幅整数型（`uint8_t`, `int16_t` など）を使い、整数オーバーフロー時の動作を理解します。

## 学習目標

- `stdint.h` で定義される固定幅整数型の使用
- 符号なし整数のオーバーフロー動作（自動wrap）
- 符号付き整数のオーバーフロー検出と飽和演算
- ビット操作の基礎

## 実装すべき関数

1. **`uint8_t add_modulo_256(uint8_t a, uint8_t b)`**
   - 2つの符号なし8ビット整数を加算
   - 256を超えた場合は自動的にwrapされる

2. **`int16_t saturate_add(int16_t a, int16_t b)`**
   - 2つの符号付き16ビット整数を加算
   - INT16_MAXを超えたらINT16_MAXで止まる
   - INT16_MINを下回ったらINT16_MINで止まる

3. **`int check_high_bit(uint8_t value)`**
   - 最上位ビット（0x80 = 128）が立っているか確認
   - 立っていれば1、立っていなければ0を返す

## テスト

```bash
colcon test --packages-select drill_c02_fixed_width_int
```

## ヒント

- `uint8_t` のオーバーフロー時の自動wrap動作はC99で保証されている
- `int` の範囲は `int16_t` より広いため、計算時は `int` にキャストしてからチェック
- ビット操作には `&` 演算子を使用
