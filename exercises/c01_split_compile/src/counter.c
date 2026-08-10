/* I AM NOT DONE
 *
 * 分割コンパイルとヘッダガードの課題です。
 * include/drill/counter.h にもやることがあります（ヘッダガード）。
 */

#include "drill/counter.h"

void counter_init(struct Counter * c, int32_t start, int32_t step)
{
  /* TODO: value と step を設定してください。 */
  (void)c;
  (void)start;
  (void)step;
}

void counter_advance(struct Counter * c)
{
  /* TODO: value に step を足してください。 */
  (void)c;
}

int32_t counter_value(const struct Counter * c)
{
  /* TODO: value を返してください。 */
  (void)c;
  return 0;
}

/* TODO: このファイルの中だけで見える変数を static で用意し、
 * 呼ばれるたびに 1 増やして返してください。
 * static をファイルスコープで使うと「他の .c から見えない」という意味になります。
 * （関数内 static の「値が残る」とは別の意味です） */
int32_t counter_call_count(void)
{
  return 0;
}
