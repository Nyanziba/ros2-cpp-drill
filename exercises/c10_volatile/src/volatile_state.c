/* I AM NOT DONE
 *
 * volatile と共有変数の課題です。
 * 「volatile はスレッドセーフではない」「最適化を防ぐだけ」を理解することが目標です。
 */

#include "drill/volatile_state.h"

/* TODO: この変数に volatile キーワードを付けてください。
 * volatile がないと、コンパイラが最適化で「毎回メモリから読まず、レジスタの値を使う」
 * という最適化をする可能性があります。
 * external からの変更を見落とします。 */
volatile state_t g_machine_state;

void machine_init(void)
{
  /* TODO: g_machine_state を STATE_IDLE に初期化してください。 */
  (void)0;
}

void machine_start(void)
{
  /* TODO: g_machine_state を STATE_RUNNING に設定してください。 */
  (void)0;
}

void machine_stop(void)
{
  /* TODO: g_machine_state を STATE_STOPPED に設定してください。 */
  (void)0;
}

state_t machine_get_state(void)
{
  /* TODO: g_machine_state の値を返してください。
   * 単純に返すだけです。volatile の変数ですが、
   * 関数の中ではシンプルに値を返します。 */
  return STATE_IDLE;
}

int machine_is_idle(void)
{
  /* TODO: g_machine_state が STATE_IDLE かどうかを返してください。
   * 1 なら TRUE、0 なら FALSE。 */
  return 0;
}

int machine_is_running(void)
{
  /* TODO: g_machine_state が STATE_RUNNING かどうかを返してください。 */
  return 0;
}

int machine_is_stopped(void)
{
  /* TODO: g_machine_state が STATE_STOPPED かどうかを返してください。 */
  return 0;
}

void machine_simulate_external_change(state_t new_state)
{
  /* TODO: 外部から g_machine_state を変更する関数です。
   * 割り込みハンドラーや別のプロセスからの変更をシミュレートします。
   * 単に g_machine_state = new_state と設定してください。 */
  (void)new_state;
}
