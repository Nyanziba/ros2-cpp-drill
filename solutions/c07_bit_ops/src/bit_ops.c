#include "drill/bit_ops.h"

/* ===== CAN ID 関数 ===== */

uint16_t can_id_create(uint8_t type, uint8_t device, uint8_t cmd)
{
  return ((uint16_t)type << 8) | ((uint16_t)device << 4) | (uint16_t)cmd;
}

uint8_t can_id_extract_type(uint16_t id)
{
  return (id >> 8) & 0x07;
}

uint8_t can_id_extract_device(uint16_t id)
{
  return (id >> 4) & 0x0F;
}

uint8_t can_id_extract_cmd(uint16_t id)
{
  return id & 0x0F;
}

uint16_t can_id_set_device(uint16_t id, uint8_t new_device)
{
  /* Device フィールド [7:4] をクリアして新しい値を設定 */
  return (id & ~0xF0) | ((uint16_t)new_device << 4);
}

/* ===== レジスタ操作関数 ===== */

uint16_t register_set_bit(uint16_t reg, uint8_t bit_pos)
{
  return reg | (1U << bit_pos);
}

uint16_t register_clear_bit(uint16_t reg, uint8_t bit_pos)
{
  return reg & ~(1U << bit_pos);
}

uint8_t register_read_bit(uint16_t reg, uint8_t bit_pos)
{
  return (reg >> bit_pos) & 1;
}

uint16_t register_set_bits(uint16_t reg, uint8_t msb_pos, uint8_t lsb_pos, uint16_t value)
{
  uint8_t width = msb_pos - lsb_pos + 1;
  uint16_t mask = (1U << width) - 1;
  return (reg & ~(mask << lsb_pos)) | ((value & mask) << lsb_pos);
}
