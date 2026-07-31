#include "drill/reader.hpp"

int read_value(const int * p)
{
  return *p;
}

void modify_value(int * p, int new_val)
{
  *p = new_val;
}

const int * get_constant_ptr(const int * p)
{
  return p;
}
