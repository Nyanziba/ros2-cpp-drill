/* このファイルは編集しません（インタフェースの提示）。 */

#ifndef DRILL_VOLATILE_STATE_H
#define DRILL_VOLATILE_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 状態遷移マシンの状態 */
typedef enum {
  STATE_IDLE = 0,
  STATE_RUNNING = 1,
  STATE_STOPPED = 2
} state_t;

/* 外部から更新される共有変数。
 * 通常の変数では、コンパイラが最適化で読み込みを削除する可能性があります。
 * volatile を使うと、毎回メモリから実際に読む必要があります。 */
extern volatile state_t g_machine_state;

/* マシンの初期化。g_machine_state を STATE_IDLE に設定します。 */
void machine_init(void);

/* マシンを実行状態に遷移させます（g_machine_state を STATE_RUNNING に設定）。 */
void machine_start(void);

/* マシンを停止状態に遷移させます（g_machine_state を STATE_STOPPED に設定）。 */
void machine_stop(void);

/* g_machine_state の現在の値を取得します。 */
state_t machine_get_state(void);

/* 現在の状態がアイドル状態かを確認します。 */
int machine_is_idle(void);

/* 現在の状態が実行状態かを確認します。 */
int machine_is_running(void);

/* 現在の状態が停止状態かを確認します。 */
int machine_is_stopped(void);

/* g_machine_state に直接書き込みをシミュレートします（外部割り込みによる変更をシミュレート）。
 * テスト用のヘルパー関数です。通常のコードでは使いません。 */
void machine_simulate_external_change(state_t new_state);

#ifdef __cplusplus
}
#endif

#endif  // DRILL_VOLATILE_STATE_H
