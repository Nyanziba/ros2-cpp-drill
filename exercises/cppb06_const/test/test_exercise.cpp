// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/config.hpp"

TEST(ConstTest, ConstCorrect)
{
  const int val = 100;
  const Config cfg(val);
  
  // const オブジェクトから get_limit() を呼べる（末尾に const が必須）
  EXPECT_EQ(cfg.get_limit(), 100);
  
  // ptr_to_limit() も const メンバ関数（末尾に const が必須）
  const int * p = cfg.ptr_to_limit();
  EXPECT_EQ(*p, 100);
}

TEST(ConstTest, 戻り値のポインタはconst)
{
  int val = 50;
  Config cfg(val);
  const int * p = cfg.ptr_to_limit();
  EXPECT_EQ(*p, 50);
  // p を通じて val を修正することはできない
}
