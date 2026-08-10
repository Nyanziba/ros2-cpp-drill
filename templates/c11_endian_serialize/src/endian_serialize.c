/* I AM NOT DONE
 *
 * エンディアンとシリアライズの課題です。
 * CAN プロトコルの速度目標コマンドを扱います。
 * include/drill/endian_serialize.h に仕様があります。
 */

#include "drill/endian_serialize.h"
#include <string.h>

void write_float_le(float value, uint8_t data[8], size_t offset)
{
  /* TODO: float value を uint32_t に変換し、
   * リトルエンディアンで 4 バイトに分解して書き込んでください。
   * 手順:
   * 1. memcpy で value を uint32_t に変換
   * 2. シフト (& 0xFF) で 1 バイトずつ取り出す
   * 3. data[offset + 0..3] に書き込む */
  (void)value;
  (void)data;
  (void)offset;
}

float read_float_le(const uint8_t data[8], size_t offset)
{
  /* TODO: リトルエンディアンで読んだ 4 バイトを float に復元してください。
   * 手順:
   * 1. 各バイトを (uint8_t) でキャストしてから widen して uint32_t を組み立てる
   * 2. memcpy で float に変換
   * 3. 返す */
  (void)data;
  (void)offset;
  return 0.0f;
}

void write_uint32_le(uint32_t value, uint8_t data[8], size_t offset)
{
  /* TODO: uint32_t value をリトルエンディアンで分解して書き込んでください。 */
  (void)value;
  (void)data;
  (void)offset;
}

uint32_t read_uint32_le(const uint8_t data[8], size_t offset)
{
  /* TODO: リトルエンディアンで読んだ 4 バイトを uint32_t に復元してください。 */
  (void)data;
  (void)offset;
  return 0;
}

void build_speed_target_command(uint8_t port_id, float target_speed, uint8_t payload[8])
{
  /* TODO: ペイロードを構築してください。
   * Byte 0: port_id
   * Byte 1-4: target_speed（float、LE）
   * Byte 5-7: 0 埋め */
  (void)port_id;
  (void)target_speed;
  (void)payload;
}
