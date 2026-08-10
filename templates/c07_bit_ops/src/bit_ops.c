/* I AM NOT DONE
 *
 * ビット演算とレジスタ操作の課題です。
 * CAN ID の組み立て・分解と、レジスタの read-modify-write を実装してください。
 */

#include "drill/bit_ops.h"

/* ===== CAN ID 関数 ===== */

uint16_t can_id_create(uint8_t type, uint8_t device, uint8_t cmd)
{
  /* TODO: type, device, cmd から 11 bit ID を組み立ててください。
   * (type << 8) | (device << 4) | cmd
   * ただし、型が uint8_t なので、使用できるビット数に注意。 */
  (void)type;
  (void)device;
  (void)cmd;
  return 0;
}

uint8_t can_id_extract_type(uint16_t id)
{
  /* TODO: id の [10:8] ビットを抽出してください。
   * (id >> 8) & 0x07 */
  (void)id;
  return 0;
}

uint8_t can_id_extract_device(uint16_t id)
{
  /* TODO: id の [7:4] ビットを抽出してください。
   * (id >> 4) & 0x0F */
  (void)id;
  return 0;
}

uint8_t can_id_extract_cmd(uint16_t id)
{
  /* TODO: id の [3:0] ビットを抽出してください。
   * id & 0x0F */
  (void)id;
  return 0;
}

uint16_t can_id_set_device(uint16_t id, uint8_t new_device)
{
  /* TODO: id の Device フィールド [7:4] を new_device に変更してください。
   * 他のビット（Type と Cmd）は変えません。
   *
   * 手順：
   * 1. Device フィールドをマスク（0 に）する：id &= ~(0x0F << 4)
   * 2. 新しい値を追加する：id |= (new_device << 4)
   * または 1 行で： id = (id & ~(0xF0)) | (new_device << 4)
   */
  (void)id;
  (void)new_device;
  return 0;
}

/* ===== レジスタ操作関数 ===== */

uint16_t register_set_bit(uint16_t reg, uint8_t bit_pos)
{
  /* TODO: reg の bit_pos ビットを 1 にしてください。
   * reg |= (1U << bit_pos) */
  (void)reg;
  (void)bit_pos;
  return 0;
}

uint16_t register_clear_bit(uint16_t reg, uint8_t bit_pos)
{
  /* TODO: reg の bit_pos ビットを 0 にしてください。
   * reg &= ~(1U << bit_pos) */
  (void)reg;
  (void)bit_pos;
  return 0;
}

uint8_t register_read_bit(uint16_t reg, uint8_t bit_pos)
{
  /* TODO: reg の bit_pos ビットを読んでください。
   * (reg >> bit_pos) & 1 */
  (void)reg;
  (void)bit_pos;
  return 0;
}

uint16_t register_set_bits(uint16_t reg, uint8_t msb_pos, uint8_t lsb_pos, uint16_t value)
{
  /* TODO: reg の [msb_pos:lsb_pos] ビットを value に設定してください。
   *
   * 手順：
   * 1. マスク幅を計算：width = msb_pos - lsb_pos + 1
   * 2. マスクを作成：mask = ((1U << width) - 1)
   * 3. 対象ビット範囲をクリア：reg &= ~(mask << lsb_pos)
   * 4. 新しい値を追加：reg |= (value & mask) << lsb_pos
   */
  (void)reg;
  (void)msb_pos;
  (void)lsb_pos;
  (void)value;
  return 0;
}
