#include "drill/macro_utils.h"

#define DEBUG 1

int is_debug_mode(void)
{
  return DEBUG;
}

int validate_port_id(uint8_t port_id)
{
  /* 有効な範囲は 1-8 */
  return (port_id >= 1 && port_id <= 8) ? 1 : 0;
}
