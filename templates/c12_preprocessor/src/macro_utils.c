/* I AM NOT DONE
 *
 * プリプロセッサとデバッグの課題です。
 * include/drill/macro_utils.h のマクロと構造体を参考にしてください。
 */

#include "drill/macro_utils.h"

/* TODO: このファイルでデバッグモードを定義してください。
 * 以下の is_debug_mode() が 1 を返すようにします。
 * マクロで DEBUG フラグを定義してください。 */

int is_debug_mode(void)
{
  /* TODO: デバッグモードなら 1、そうでなければ 0 を返してください。
   * マクロを使う方法と、コンパイル時フラグを使う方法があります。 */
  return 0;
}

int validate_port_id(uint8_t port_id)
{
  /* TODO: ポート ID が 1-8 の範囲内かチェックしてください。
   * 無効なら assert で通知、有効なら 1 を返してください。
   * マクロの引数は 2 回評価されることに注意してください。 */
  (void)port_id;
  return 0;
}
