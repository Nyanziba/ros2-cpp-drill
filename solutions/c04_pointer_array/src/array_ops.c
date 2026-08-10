#include "drill/array_ops.h"

int sum_array(const int *arr, size_t len)
{
  /* len が 0 の場合は 0 を返す */
  if (len == 0) {
    return 0;
  }

  int sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum += arr[i];
  }

  return sum;
}

int max_element(const int *arr, size_t len)
{
  /* len が 0 の場合は INT_MIN を返す */
  if (len == 0) {
    return INT_MIN;
  }

  int max = arr[0];
  for (size_t i = 1; i < len; i++) {
    if (arr[i] > max) {
      max = arr[i];
    }
  }

  return max;
}

void double_elements(int *arr, size_t len)
{
  /* len が 0 の場合は何もしない */
  for (size_t i = 0; i < len; i++) {
    arr[i] *= 2;
  }
}
