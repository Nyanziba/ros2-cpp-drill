#include "drill/array_util.hpp"

int sum(const int * data, std::size_t count)
{
  int total = 0;
  for (std::size_t i = 0; i < count; ++i) {
    total += data[i];
  }
  return total;
}

const int * find_first(const int * data, std::size_t count, int target)
{
  for (std::size_t i = 0; i < count; ++i) {
    if (data[i] == target) {
      return &data[i];
    }
  }
  return nullptr;
}
