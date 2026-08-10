# c04: 配列とポインタ演算

配列とポインタの関係、そしてメモリ安全性を学びます。このコースはASAN（AddressSanitizer）有効化されているため、バッファオーバーフローが即座に検出されます。

## 学習目標

- 配列とポインタの関係
- ポインタ演算による要素アクセス
- 要素数を安全に処理する方法
- ASAN（AddressSanitizer）による実行時検出

## 実装すべき関数

1. **`int sum_array(const int *arr, size_t len)`**
   - 配列の先頭からlen個の要素の合計を計算
   - lenが0の場合は0を返す
   - const修飾子により読み取り専用

2. **`int max_element(const int *arr, size_t len)`**
   - 配列の先頭からlen個の要素の最大値を返す
   - lenが0の場合はINT_MINを返す

3. **`void double_elements(int *arr, size_t len)`**
   - 配列の先頭からlen個の要素をそれぞれ2倍にする
   - lenが0の場合は何もしない

## テスト

```bash
colcon test --packages-select drill_c04_pointer_array
```

## 重要な注意

- **ASAN有効**: このプロジェクトはASAN（AddressSanitizer）で保護されており、配列の範囲外アクセスがあると即座にプログラムが停止します
- **安全な実装**: `len` パラメータに基づいて、必ず `i < len` などで範囲チェックしてください
- 未完成版での実行時、ASAN検出でテストが失敗するのは正常な挙動です

## ヒント

- ポインタ演算：`arr[i]` は `*(arr + i)` と同じ
- `size_t` は符号なし整数型で、配列インデックスに適切
- constで修飾された引数は読み取り専用
