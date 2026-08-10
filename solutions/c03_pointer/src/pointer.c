#include <stddef.h>
#include "drill/pointer.h"

int swap_values(int *a, int *b)
{
  /* NULL チェック */
  if (a == NULL || b == NULL) {
    return -1;
  }

  /* 一時変数を使って交換 */
  int temp = *a;
  *a = *b;
  *b = temp;

  return 0;
}

int multiply(int x, int *result)
{
  /* NULL チェック */
  if (result == NULL) {
    return -1;
  }

  /* 計算結果をポインタ経由で書き込む */
  *result = x * 2;

  return 0;
}

int triple_pointer(int *p)
{
  /* NULL チェック */
  if (p == NULL) {
    return -1;
  }

  /* 指す先の値を 3倍にする */
  *p *= 3;

  return 0;
}
