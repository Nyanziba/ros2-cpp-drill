# c05: 文字列

C言語の文字列処理を学びます。C言語では文字列はNUL終端（`'\0'`）されたバイト列です。

## 学習目標

- NUL終端文字列の構造理解
- バッファサイズの管理
- 文字列の安全なコピーと連結
- 標準ライブラリ関数（`strlen`, `strncpy`, `strncat`）の安全な実装

## 実装すべき関数

1. **`int string_length(const char *s)`**
   - C文字列sの長さを返す（NUL文字は含まない）
   - sがNULLの場合は-1を返す
   - `strlen()` の実装

2. **`int string_copy(char *dest, const char *src, size_t max_len)`**
   - srcをdestにコピーする
   - destのバッファサイズはmax_lenバイト
   - コピー後、必ずNUL終端文字を付加（max_lenに含める）
   - srcがNUL、destがNUL、またはmax_lenが0なら-1を返す
   - 成功時は0を返す

3. **`int string_concat(char *dest, const char *src, size_t max_len)`**
   - srcをdestに追加（連結）する
   - destは既にNUL終端された文字列と仮定
   - destのバッファサイズはmax_lenバイト（NULを含む）
   - 連結後も必ずNUL終端文字を付加
   - destがNUL、srcがNUL、またはmax_lenが0なら-1を返す
   - 成功時は0を返す

## テスト

```bash
colcon test --packages-select drill_c05_string
```

## 重要な注意

- **NUL終端は必須**: すべての文字列操作後、必ず `'\0'` で終端させる
- **バッファサイズの管理**: 最大`max_len - 1`の文字がコピーまたは追加されます（残り1バイトはNUL用）
- **安全性**: `strlen()`, `strcpy()`, `strcat()` より安全な実装を目指す

## ヒント

- ポインタをインクリメント：`p++` で次の文字へ移動
- 文字列の終端判定：`s[i] != '\0'` または `*s != '\0'`
- バッファ容量の計算：コピー対象の長さ+1（NUL用）がmax_len以下か確認
