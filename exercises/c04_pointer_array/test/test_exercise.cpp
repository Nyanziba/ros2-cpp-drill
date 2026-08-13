// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include <climits>
#include <cstring>

#include "drill/array_ops.h"

TEST(ArrayOpsTest, 配列の合計を計算する)
{
  int arr[] = {1, 2, 3, 4, 5};
  EXPECT_EQ(sum_array(arr, 5), 15);

  int arr2[] = {-5, -3, 8};
  EXPECT_EQ(sum_array(arr2, 3), 0);

  int arr3[] = {0};
  EXPECT_EQ(sum_array(arr3, 1), 0);
}

TEST(ArrayOpsTest, 空配列の合計はゼロ)
{
  EXPECT_EQ(sum_array(nullptr, 0), 0);
}

TEST(ArrayOpsTest, 配列の最大値を見つける)
{
  int arr[] = {1, 5, 3, 2, 4};
  EXPECT_EQ(max_element(arr, 5), 5);

  int arr2[] = {-10, -5, -20};
  EXPECT_EQ(max_element(arr2, 3), -5);

  int arr3[] = {42};
  EXPECT_EQ(max_element(arr3, 1), 42);
}

TEST(ArrayOpsTest, 空配列の最大値はINT_MIN)
{
  EXPECT_EQ(max_element(nullptr, 0), INT_MIN);
}

TEST(ArrayOpsTest, 配列の要素を2倍にする)
{
  int arr[] = {1, 2, 3, 4, 5};
  double_elements(arr, 5);
  int expected[] = {2, 4, 6, 8, 10};
  for (size_t i = 0; i < 5; i++) {
    EXPECT_EQ(arr[i], expected[i]);
  }
}

TEST(ArrayOpsTest, 負の数も2倍にできる)
{
  int arr[] = {-3, 5, -1};
  double_elements(arr, 3);
  EXPECT_EQ(arr[0], -6);
  EXPECT_EQ(arr[1], 10);
  EXPECT_EQ(arr[2], -2);
}

TEST(ArrayOpsTest, 空配列は何もしない)
{
  // nullptr 渡しでも segfault しないはず
  double_elements(nullptr, 0);
}
