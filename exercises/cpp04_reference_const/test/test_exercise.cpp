// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/copycounter.hpp"

TEST(ReferenceConstTest, ConvertsToUppercaseWithConstReference)
{
  CopyCounter cc;
  std::string result = cc.copy_and_uppercase("hello");
  EXPECT_EQ(result, "HELLO");
}

TEST(ReferenceConstTest, ConstMemberFunctionReturnsDescription)
{
  const CopyCounter cc;
  std::string desc = cc.get_description();
  EXPECT_EQ(desc, "コピー回数: 0");
}
