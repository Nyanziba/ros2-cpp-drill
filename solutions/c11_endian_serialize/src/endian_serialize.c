#include "drill/endian_serialize.h"
#include <string.h>

void write_float_le(float value, uint8_t data[8], size_t offset)
{
  uint32_t u32;
  memcpy(&u32, &value, sizeof(u32));

  data[offset + 0] = u32 & 0xFF;
  data[offset + 1] = (u32 >> 8) & 0xFF;
  data[offset + 2] = (u32 >> 16) & 0xFF;
  data[offset + 3] = (u32 >> 24) & 0xFF;
}

float read_float_le(const uint8_t data[8], size_t offset)
{
  uint32_t u32 = ((uint32_t)data[offset + 0]) |
                 (((uint32_t)data[offset + 1]) << 8) |
                 (((uint32_t)data[offset + 2]) << 16) |
                 (((uint32_t)data[offset + 3]) << 24);

  float f;
  memcpy(&f, &u32, sizeof(f));
  return f;
}

void write_uint32_le(uint32_t value, uint8_t data[8], size_t offset)
{
  data[offset + 0] = value & 0xFF;
  data[offset + 1] = (value >> 8) & 0xFF;
  data[offset + 2] = (value >> 16) & 0xFF;
  data[offset + 3] = (value >> 24) & 0xFF;
}

uint32_t read_uint32_le(const uint8_t data[8], size_t offset)
{
  return ((uint32_t)data[offset + 0]) |
         (((uint32_t)data[offset + 1]) << 8) |
         (((uint32_t)data[offset + 2]) << 16) |
         (((uint32_t)data[offset + 3]) << 24);
}

void build_speed_target_command(uint8_t port_id, float target_speed, uint8_t payload[8])
{
  memset(payload, 0, 8);
  payload[0] = port_id;
  write_float_le(target_speed, payload, 1);
}
