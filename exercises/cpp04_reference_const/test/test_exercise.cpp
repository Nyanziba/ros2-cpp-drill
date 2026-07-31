// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/copycounter.hpp"

TEST(ReferenceConstTest, constReferencePolicyで大文字に変換)
{
  CopyCounter cc;
  std::string result = cc.copy_and_uppercase("hello");
  EXPECT_EQ(result, "HELLO");
}

TEST(ReferenceConstTest, constMemberFunctionが説明を返す)
{
  const CopyCounter cc;
  std::string desc = cc.get_description();
  EXPECT_EQ(desc, "コピー回数: 0");
}
