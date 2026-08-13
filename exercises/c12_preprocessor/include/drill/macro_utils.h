/* このファイルは編集しません（インタフェースの提示）。 */

#include <stdint.h>
#include <assert.h>

/* マクロの罠を学ぶ課題。
 * 以下のマクロを正しく実装してください。 */

/* 求積マクロ（簡単版） */
#define SQUARE(x) (x) * (x)

/* SQ の実装で、複文マクロが必要になる場合に対応 */
#define DOUBLE_SQ(x) do { (x) = (x) * (x); } while (0)

/* CAN ペイロード構造体（8 バイト固定） */
typedef struct {
  uint8_t port_id;
  uint8_t data[7];
} CanPayload;

/* このアサーションが通らなければコンパイルエラーになります。
 * （C11 の _Static_assert） */
#ifdef __cplusplus
static_assert(sizeof(CanPayload) == 8, "CanPayload must be 8 bytes");
#else
_Static_assert(sizeof(CanPayload) == 8, "CanPayload must be 8 bytes");
#endif

/* C++ の gtest から C の関数を呼ぶための宣言。 */
#ifdef __cplusplus
extern "C" {
#endif

/* デバッグモードかどうかを返す関数 */
int is_debug_mode(void);

/* ポート ID の妥当性をチェック（1-8 のみ有効）
 * 無効なら assert で通知、有効なら 1 を返す */
int validate_port_id(uint8_t port_id);

#ifdef __cplusplus
}
#endif
