#include "drill/swapper.hpp"

void swap_values(int & a, int & b)
{
  int temp = a;
  a = b;
  b = temp;
}

int & largest(int & a, int & b)
{
  // > ではなく >= にしてあります。
  // > だと a == b のときに b が返ってしまい、仕様と食い違います。
  return (a >= b) ? a : b;
}
