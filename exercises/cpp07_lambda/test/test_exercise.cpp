// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/callbackmanager.hpp"

TEST(LambdaTest, コールバックが呼び出される)
{
  CallbackManager mgr;
  int result = 0;

  mgr.register_callback([&result](int x) { result += x; });
  mgr.fire(10);

  EXPECT_EQ(result, 10);
}

TEST(LambdaTest, 複数のコールバックが登録できる)
{
  CallbackManager mgr;
  int result1 = 0, result2 = 0;

  mgr.register_callback([&result1](int x) { result1 = x * 2; });
  mgr.register_callback([&result2](int x) { result2 = x + 5; });

  mgr.fire(3);

  EXPECT_EQ(result1, 6);
  EXPECT_EQ(result2, 8);
}
