#include "drill/fixed_width.h"
#include <limits.h>

uint8_t add_modulo_256(uint8_t a, uint8_t b)
{
  /* uint8_t は自動的にオーバーフロー時に wrap される。
   * これは C99 で保証されている動作です（符号なし整数型について）。 */
  return a + b;
}

int16_t saturate_add(int16_t a, int16_t b)
{
  /* 加算がオーバーフロー・アンダーフローする場合は飽和。
   * int の範囲が int16_t より広いため、int にキャストして計算してから
   * 結果が範囲内かチェックします。 */
  int result = (int)a + (int)b;

  if (result > INT16_MAX) {
    return INT16_MAX;
  } else if (result < INT16_MIN) {
    return INT16_MIN;
  } else {
    return (int16_t)result;
  }
}

int check_high_bit(uint8_t value)
{
  /* 最上位ビットは 0x80 = 128。
   * ビット AND を使ってチェック。 */
  return (value & 0x80) != 0 ? 1 : 0;
}
