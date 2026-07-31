#include "drill/checker.hpp"

bool try_read(const int * p, int * out)
{
  if (p == nullptr || out == nullptr) {
    return false;
  }
  *out = *p;
  return true;
}
