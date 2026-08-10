#include "drill/struct_align.h"

size_t get_sizeof_point2d(void)
{
  return sizeof(struct Point2D);
}

size_t get_offset_point2d_x(void)
{
  return offsetof(struct Point2D, x);
}

size_t get_offset_point2d_y(void)
{
  return offsetof(struct Point2D, y);
}

size_t get_sizeof_rgb(void)
{
  return sizeof(struct RGB);
}

size_t get_offset_rgb_r(void)
{
  return offsetof(struct RGB, r);
}

size_t get_offset_rgb_g(void)
{
  return offsetof(struct RGB, g);
}

size_t get_offset_rgb_b(void)
{
  return offsetof(struct RGB, b);
}

size_t get_sizeof_packed_data(void)
{
  return sizeof(struct PackedData);
}

size_t get_offset_packed_data_flag(void)
{
  return offsetof(struct PackedData, flag);
}

size_t get_offset_packed_data_id(void)
{
  return offsetof(struct PackedData, id);
}

size_t get_offset_packed_data_counter(void)
{
  return offsetof(struct PackedData, counter);
}
