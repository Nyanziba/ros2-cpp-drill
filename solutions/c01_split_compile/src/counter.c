#include "drill/counter.h"

void counter_init(struct Counter * c, int32_t start, int32_t step)
{
  c->value = start;
  c->step = step;
}

void counter_advance(struct Counter * c)
{
  c->value += c->step;
}

int32_t counter_value(const struct Counter * c)
{
  return c->value;
}

/* ファイルスコープの static。
 * この .c の外からは名前で参照できない（内部リンケージ）。
 * 関数内 static の「値が残る」とは別の意味であることに注意。 */
static int32_t call_count = 0;

int32_t counter_call_count(void)
{
  call_count += 1;
  return call_count;
}
