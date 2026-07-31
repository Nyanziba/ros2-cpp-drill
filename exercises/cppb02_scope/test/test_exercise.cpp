// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/scope.hpp"

TEST(ScopeTest, スコープを抜けるとき逆順に破棄される)
{
  g_trace_log.clear();
  trace_something();
  
  // outer, inner が construct されて、inner, outer の順に destruct
  std::string expected = "outer:construct:inner:construct:inner:destruct:outer:destruct:";
  EXPECT_EQ(g_trace_log, expected);
}
