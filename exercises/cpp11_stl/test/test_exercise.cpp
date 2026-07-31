// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/limiter.hpp"

TEST(StlTest, clampが値を制限する)
{
  EXPECT_DOUBLE_EQ(clamp_velocity(5.0, 0.0, 10.0), 5.0);
  EXPECT_DOUBLE_EQ(clamp_velocity(-5.0, 0.0, 10.0), 0.0);
  EXPECT_DOUBLE_EQ(clamp_velocity(15.0, 0.0, 10.0), 10.0);
}

TEST(StlTest, optionalで値を見つける)
{
  std::map<std::string, int> users = {
    {"Alice", 1},
    {"Bob", 2},
    {"Charlie", 3}
  };

  auto alice_id = find_user_id(users, "Alice");
  EXPECT_TRUE(alice_id.has_value());
  EXPECT_EQ(*alice_id, 1);

  auto unknown = find_user_id(users, "Unknown");
  EXPECT_FALSE(unknown.has_value());
}
