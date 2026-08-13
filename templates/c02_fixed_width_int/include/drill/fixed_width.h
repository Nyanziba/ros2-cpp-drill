/* このファイルは編集しません */

#ifndef DRILL_FIXED_WIDTH_H
#define DRILL_FIXED_WIDTH_H

#include <stdint.h>

/* C++ の gtest から C の関数を呼ぶための宣言。
 * これが無いと C++ 側が名前マングリングした記号を探してしまい、
 * undefined reference でリンクに失敗します。 */
#ifdef __cplusplus
extern "C" {
#endif

/* 符号なし整数の加算。オーバーフローは自動的に wrap される。
 * 例: 200 + 100 = 44 (% 256) */
uint8_t add_modulo_256(uint8_t a, uint8_t b);

/* 符号付き整数の加算。オーバーフローは INT16_MAX または INT16_MIN で飽和する。
 * 例: INT16_MAX + 1000 = INT16_MAX */
int16_t saturate_add(int16_t a, int16_t b);

/* 最上位ビット（最も値の大きいビット）が立っているかチェック。
 * 立っていれば 1、立っていなければ 0 を返す。
 * 例: 0x80 (10000000) -> 1, 0x7F (01111111) -> 0 */
int check_high_bit(uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* DRILL_FIXED_WIDTH_H */
