/* このファイルは編集しません（インタフェースの提示）。 */

#include <stdint.h>
#include <stddef.h>

/* C++ の gtest から C の関数を呼ぶための宣言。
 * これが無いと C++ 側が名前マングリングした記号を探してしまい、
 * undefined reference でリンクに失敗します。
 * （このブロックは既に書いてあります。消さないでください） */
#ifdef __cplusplus
extern "C" {
#endif

/* float をリトルエンディアンで 4 バイトに分解し、
 * 指定されたオフセットから data[] に書き込みます。
 * offset + 4 <= 8 であることを前提とします。
 *
 * 実装のルール:
 * 1. float を uint32_t に変換するときは、ポインタキャストではなく memcpy を使う
 * 2. uint32_t をバイト単位にシフトして分解する */
void write_float_le(float value, uint8_t data[8], size_t offset);

/* リトルエンディアンで読んだ 4 バイト（offset から offset+3）を
 * float に復元します。offset + 4 <= 8 であることを前提とします。
 *
 * 実装のルール:
 * 1. char/signed char が負の値でも壊れないよう、各バイトを (uint8_t) でキャストしてから widen する
 * 2. uint32_t に組み立てる
 * 3. memcpy で float に変換する */
float read_float_le(const uint8_t data[8], size_t offset);

/* uint32_t をリトルエンディアンで 4 バイトに分解し、
 * 指定されたオフセットから data[] に書き込みます。
 * offset + 4 <= 8 であることを前提とします。 */
void write_uint32_le(uint32_t value, uint8_t data[8], size_t offset);

/* リトルエンディアンで読んだ 4 バイト（offset から offset+3）を
 * uint32_t に復元します。offset + 4 <= 8 であることを前提とします。 */
uint32_t read_uint32_le(const uint8_t data[8], size_t offset);

/* 速度目標コマンドを構築します。
 * ペイロードは 8 バイト。
 * Byte 0: ポート ID（1-8、ここでは 1-8 の値をそのまま使う）
 * Byte 1-4: 目標速度（float、LE）
 * Byte 5-7: 0 埋め */
void build_speed_target_command(uint8_t port_id, float target_speed, uint8_t payload[8]);

#ifdef __cplusplus
}
#endif
