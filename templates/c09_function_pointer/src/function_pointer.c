/* I AM NOT DONE
 *
 * 関数ポインタとテーブル駆動型プログラミングの課題です。
 */

#include "drill/function_pointer.h"

#include <stdlib.h>

struct LedController {
  /* TODO: LED_COUNT 個の関数ポインタを保持する配列を追加してください。
   * typedef led_callback_t を使い、NULL で初期化します。 */
  led_callback_t handlers[LED_COUNT];
};

struct LedController * led_controller_create(void)
{
  /* TODO: LedController を malloc で確保し、すべてのハンドラーを NULL で初期化してください。 */
  (void)0;
  return NULL;
}

void led_controller_destroy(struct LedController * ctrl)
{
  /* TODO: 渡されたポインタを free で解放してください。
   * NULL チェックは呼び出し側で行うので、ここでは不要です。 */
  (void)ctrl;
}

void led_register_handler(struct LedController * ctrl, int led_id, led_callback_t handler)
{
  /* TODO: ctrl->handlers[led_id] に handler を代入してください。
   * led_id の範囲チェックはここでは行いません（信頼できるコーダーを仮定）。 */
  (void)ctrl;
  (void)led_id;
  (void)handler;
}

void led_set(struct LedController * ctrl, int led_id, int state)
{
  /* TODO: handlers[led_id] が NULL でなければ、それを呼び出してください。
   * 呼び出しは (*handler)(led_id, state) の形です。
   * NULL なら何もしません（未登録状態）。 */
  (void)ctrl;
  (void)led_id;
  (void)state;
}

int led_handler_is_null(const struct LedController * ctrl, int led_id)
{
  /* TODO: handlers[led_id] が NULL なら 1、そうでなければ 0 を返してください。 */
  (void)ctrl;
  (void)led_id;
  return 0;
}
