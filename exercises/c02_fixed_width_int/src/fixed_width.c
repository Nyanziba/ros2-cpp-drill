/* I AM NOT DONE
 *
 * 固定幅整数と暗黙変換の課題です。
 * uint8_t, int16_t などの型を使い、オーバーフローと飽和演算を理解します。
 */

#include "drill/fixed_width.h"

/* TODO: 符号なし 8 ビット整数の加算。
 * 256 を超えた分は自動的に wrap されます。
 * 例: 200 + 100 = 300 = 44 (% 256) */
uint8_t add_modulo_256(uint8_t a, uint8_t b)
{
  (void)a;
  (void)b;
  return 0;
}

/* TODO: 符号付き 16 ビット整数の加算。
 * 値が INT16_MAX を超えたら INT16_MAX に、INT16_MIN を下回ったら INT16_MIN に止まります。
 * (void) で警告を消していますが、実装時は削除してください。
 * ヒント: INT16_MAX, INT16_MIN は limits.h で定義されています。 */
int16_t saturate_add(int16_t a, int16_t b)
{
  (void)a;
  (void)b;
  return 0;
}

/* TODO: uint8_t 型の最上位ビット（0x80 = 128）が立っているかチェック。
 * 立っていれば 1、立っていなければ 0 を返します。
 * ビット演算 & を使ってください。 */
int check_high_bit(uint8_t value)
{
  (void)value;
  return 0;
}
