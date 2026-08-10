/* このファイルは編集しません（インタフェースの提示）。 */

#ifndef DRILL_STRUCT_ALIGN_H
#define DRILL_STRUCT_ALIGN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== struct の定義 =====
 * これらは修正前と修正後で同じですが、
 * テストが sizeof と offsetof で値を検証します。
 * 受講者は src/struct_align.c でこれらの値を出力する関数を実装してください。
 */

/* 構造体A（パディングあり）*/
struct Point2D {
  int16_t x;
  int16_t y;
};

/* 構造体B（パディングなし）*/
struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

/* 構造体C（メンバ順序でパディング量が変わる）*/
struct PackedData {
  uint8_t flag;
  uint64_t id;
  uint16_t counter;
};

/* ===== 計測関数 =====
 * テストは sizeof を直接呼べるので、
 * 検証用に各構造体のサイズとオフセットを返す関数を実装
 */

size_t get_sizeof_point2d(void);
size_t get_offset_point2d_x(void);
size_t get_offset_point2d_y(void);

size_t get_sizeof_rgb(void);
size_t get_offset_rgb_r(void);
size_t get_offset_rgb_g(void);
size_t get_offset_rgb_b(void);

size_t get_sizeof_packed_data(void);
size_t get_offset_packed_data_flag(void);
size_t get_offset_packed_data_id(void);
size_t get_offset_packed_data_counter(void);

#ifdef __cplusplus
}
#endif

#endif /* DRILL_STRUCT_ALIGN_H */
