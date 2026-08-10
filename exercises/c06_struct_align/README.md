# c06 構造体とアラインメント 〔C言語編〕

メモリ上で構造体がどのように配置されるか、パディングがなぜ生じるかを理解する課題です。

## ヘッダは既に完成しています

`include/drill/struct_align.h` は**編集しません**。3 つの構造体が定義されています：

```c
struct Point2D {
  int16_t x;
  int16_t y;
};

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct PackedData {
  uint8_t flag;
  uint64_t id;
  uint16_t counter;
};
```

## やること

編集するのは `src/struct_align.c` だけです。
各関数で、**対応する構造体の `sizeof` と `offsetof` を返してください。**

```c
size_t get_sizeof_point2d(void);        // sizeof(struct Point2D)
size_t get_offset_point2d_x(void);      // offsetof(struct Point2D, x)
size_t get_offset_point2d_y(void);      // offsetof(struct Point2D, y)
// ... RGB と PackedData も同様
```

C99 の `<stddef.h>` から `offsetof` マクロを使ってください。

## 動かしてみる

```bash
./drill run c06
```

## つまずきポイント

- **アラインメント** — CPU は特定のアドレス境界から値を読み取るため、各メンバのオフセットは型のサイズの倍数になることが多いです
- **パディング** — メンバの間に隙間が入り、`sizeof` が「全メンバのサイズ合計」を超えることがあります
- `struct PackedData` は小さいメンバと大きいメンバが混在します。並び順を意識してください

## テスト

| テスト | 見ているところ |
| --- | --- |
| `Point2D_のサイズ` | int16_t × 2 個の構造体がいくつのバイトになるか |
| `RGB_のサイズ` | uint8_t × 3 個の場合（パディングなし） |
| `PackedData_のサイズ` | 小さい型と大きい型の混在でパディングがどう入るか |
| `*_のオフセット` | 各メンバが実際にどのバイト位置に配置されているか |

## 参考

- [6. 構造体とアラインメント](../../docs/c/06_構造体とアラインメント.md)
