/* このファイルは編集します。ヘッダガードが抜けています。 */

/* TODO(1) ヘッダガードを入れてください。
 * いまのままだと、このヘッダを 2 回 include した翻訳単位で
 * struct Counter が二重に定義され、コンパイルが止まります。
 *
 * #ifndef DRILL_COUNTER_H
 * #define DRILL_COUNTER_H
 *   ... 中身 ...
 * #endif
 */

#include <stdint.h>

struct Counter {
  int32_t value;
  int32_t step;
};

/* C++ の gtest から C の関数を呼ぶための宣言。
 * これが無いと C++ 側が名前マングリングした記号を探してしまい、
 * undefined reference でリンクに失敗します。
 * （このブロックは既に書いてあります。消さないでください） */
#ifdef __cplusplus
extern "C" {
#endif

void counter_init(struct Counter * c, int32_t start, int32_t step);
void counter_advance(struct Counter * c);
int32_t counter_value(const struct Counter * c);

/* TODO(2) この関数を src/counter.c で実装してください。
 * 呼ばれた回数を返します。1 回目は 1、2 回目は 2、…
 * ファイルスコープの static 変数を使ってください。 */
int32_t counter_call_count(void);

#ifdef __cplusplus
}
#endif
