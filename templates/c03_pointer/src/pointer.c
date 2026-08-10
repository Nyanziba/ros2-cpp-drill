/* I AM NOT DONE
 *
 * ポインタでアドレスを渡す課題です。
 * & や * を使ってポインタ操作を理解し、NULL チェックを実装します。
 */

#include <stddef.h>
#include "drill/pointer.h"

/* TODO: 2 つの変数の値を入れ替える。
 * a と b がともに非 NULL のときに交換し、0 を返してください。
 * a または b が NULL なら -1 を返してください。
 *
 * ヒント: 一時変数を使うか、XOR swap を使うか、お好みで。
 * ただし XOR swap は型に依存するので、単純な方法をお勧めします。 */
int swap_values(int *a, int *b)
{
  (void)a;
  (void)b;
  return 0;
}

/* TODO: x を 2倍にして result ポインタが指す場所に格納する。
 * result が NULL なら -1、成功時は 0 を返してください。
 *
 * ヒント: *result = x * 2; という形で値を格納します。 */
int multiply(int x, int *result)
{
  (void)x;
  (void)result;
  return 0;
}

/* TODO: p が指す値を 3倍にする。
 * p が NULL なら -1、成功時は 0 を返してください。
 *
 * ヒント: *p *= 3; のような形で変更できます。 */
int triple_pointer(int *p)
{
  (void)p;
  return 0;
}
