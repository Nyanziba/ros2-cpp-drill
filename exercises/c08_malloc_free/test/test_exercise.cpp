// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/malloc_free.h"

TEST(DynamicArrayTest, 配列を確保できる)
{
  int * arr = create_array(10);
  ASSERT_NE(arr, nullptr);

  // テスト用に値を書き込んでみる
  arr[0] = 42;
  arr[9] = 99;
  EXPECT_EQ(arr[0], 42);
  EXPECT_EQ(arr[9], 99);

  free_array(arr);
}

TEST(DynamicArrayTest, 異なるサイズで動く)
{
  int * arr1 = create_array(1);
  int * arr2 = create_array(100);
  int * arr3 = create_array(1000);

  ASSERT_NE(arr1, nullptr);
  ASSERT_NE(arr2, nullptr);
  ASSERT_NE(arr3, nullptr);

  arr1[0] = 1;
  arr2[50] = 50;
  arr3[500] = 500;

  EXPECT_EQ(arr1[0], 1);
  EXPECT_EQ(arr2[50], 50);
  EXPECT_EQ(arr3[500], 500);

  free_array(arr1);
  free_array(arr2);
  free_array(arr3);
}

TEST(LinkedListTest, リストを作成できる)
{
  struct Node * head = create_linked_list(5);
  ASSERT_NE(head, nullptr);

  // リストをたどって値を確認
  struct Node * current = head;
  for (int i = 0; i < 5; i++) {
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(current->value, i);
    current = current->next;
  }
  EXPECT_EQ(current, nullptr);  // 最後の next は NULL

  free_linked_list(head);
}

TEST(LinkedListTest, 長さ1のリスト)
{
  struct Node * head = create_linked_list(1);
  ASSERT_NE(head, nullptr);
  EXPECT_EQ(head->value, 0);
  EXPECT_EQ(head->next, nullptr);

  free_linked_list(head);
}

TEST(LinkedListTest, 長さ10のリスト)
{
  struct Node * head = create_linked_list(10);
  ASSERT_NE(head, nullptr);

  struct Node * current = head;
  for (int i = 0; i < 10; i++) {
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(current->value, i);
    current = current->next;
  }
  EXPECT_EQ(current, nullptr);

  free_linked_list(head);
}

TEST(LinkedListTest, 複数のリストを独立して管理)
{
  struct Node * list1 = create_linked_list(3);
  struct Node * list2 = create_linked_list(5);

  // list1 の確認
  EXPECT_EQ(list1->value, 0);
  EXPECT_EQ(list1->next->value, 1);

  // list2 の確認
  EXPECT_EQ(list2->value, 0);
  EXPECT_EQ(list2->next->next->value, 2);

  free_linked_list(list1);
  free_linked_list(list2);
}
