// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include <sstream>

#include "drill/registry.hpp"

TEST(SmartPointersTest, 生きているItemだけが出力される)
{
  Registry reg;

  {
    auto item1 = std::make_shared<Item>(1, "Alpha");
    reg.add(item1);
  }  // item1 のスコープを抜ける

  auto item2 = std::make_shared<Item>(2, "Beta");
  reg.add(item2);

  // 標準出力をキャプチャして確認
  testing::internal::CaptureStdout();
  reg.fire();
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("id=2"), std::string::npos) << "Item 2 (Beta) が出力されるべき";
  EXPECT_EQ(output.find("id=1"), std::string::npos) << "Item 1 は削除されたので出力されないべき";
}
