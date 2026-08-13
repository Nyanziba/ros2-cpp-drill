/* ヘッダガード。2 回 include されても中身が 1 回しか展開されない。 */
#ifndef DRILL_COUNTER_H
#define DRILL_COUNTER_H

#include <stdint.h>

struct Counter {
  int32_t value;
  int32_t step;
};

#ifdef __cplusplus
extern "C" {
#endif

void counter_init(struct Counter * c, int32_t start, int32_t step);
void counter_advance(struct Counter * c);
int32_t counter_value(const struct Counter * c);
int32_t counter_call_count(void);

#ifdef __cplusplus
}
#endif

#endif /* DRILL_COUNTER_H */
