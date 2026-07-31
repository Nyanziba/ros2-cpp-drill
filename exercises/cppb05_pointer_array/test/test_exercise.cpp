// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/array_util.hpp"

TEST(ArrayTest, Sum)
{
  int arr[] = {1, 2, 3, 4, 5};
  EXPECT_EQ(sum(arr, 5), 15);
}

TEST(ArrayTest, Sum空の配列)
{
  int arr[] = {1};
  EXPECT_EQ(sum(arr, 0), 0);
}

TEST(ArrayTest, FindFirst見つかる)
{
  int arr[] = {10, 20, 30, 40};
  const int * p = find_first(arr, 4, 30);
  EXPECT_EQ(*p, 30);
  EXPECT_EQ(p, &arr[2]);
}

TEST(ArrayTest, FindFirst見つからない)
{
  int arr[] = {10, 20, 30, 40};
  const int * p = find_first(arr, 4, 99);
  EXPECT_EQ(p, nullptr);
}
