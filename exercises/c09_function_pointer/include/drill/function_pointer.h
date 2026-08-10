/* このファイルは編集しません（インタフェースの提示）。 */

#ifndef DRILL_FUNCTION_POINTER_H
#define DRILL_FUNCTION_POINTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* LED コントローラーの不透明な型。 */
struct LedController;

/* LED コントローラーを作成します。
 * NULL ポインタに対して呼ぶことはできません。 */
struct LedController * led_controller_create(void);

/* LED コントローラーを破棄します。
 * NULL ポインタに対して呼ぶことはできません。 */
void led_controller_destroy(struct LedController * ctrl);

/* コールバック関数型。state は 0 (OFF) または 1 (ON)。 */
typedef void (*led_callback_t)(int led_id, int state);

/* LED の ON/OFF ハンドラーを登録します。
 * led_id は 0 から LED_COUNT-1。
 * handler が NULL の場合、そのハンドラーを削除（未登録状態に）します。 */
void led_register_handler(struct LedController * ctrl, int led_id, led_callback_t handler);

/* LED を制御します。state は 0 (OFF) または 1 (ON)。
 * 登録されているハンドラーがあれば呼びます。
 * NULL（未登録）なら何もしません（安全）。 */
void led_set(struct LedController * ctrl, int led_id, int state);

/* 現在のハンドラーが NULL であるかを確認します。 */
int led_handler_is_null(const struct LedController * ctrl, int led_id);

#define LED_COUNT 4

#ifdef __cplusplus
}
#endif

#endif  // DRILL_FUNCTION_POINTER_H
