// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "drill/logger_factory.hpp"

TEST(FactoryMethodTest, createがロガーを返し書き込める)
{
  MemoryLoggerFactory factory;

  std::unique_ptr<Logger> logger = factory.create("motor");
  ASSERT_NE(logger, nullptr);
  EXPECT_EQ(logger->tag(), "motor");

  logger->write("duty=0.5");
  logger->write("duty=0.0");

  auto * memory_logger = dynamic_cast<MemoryLogger *>(logger.get());
  ASSERT_NE(memory_logger, nullptr);
  ASSERT_EQ(memory_logger->lines().size(), 2u);
  EXPECT_EQ(memory_logger->lines()[0], "[motor] duty=0.5");
  EXPECT_EQ(memory_logger->lines()[1], "[motor] duty=0.0");
}

TEST(FactoryMethodTest, 成功した生成だけが順番に登録される)
{
  MemoryLoggerFactory factory;

  auto first = factory.create("motor");
  auto second = factory.create("imu");
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);

  const std::vector<std::string> expected{"motor", "imu"};
  EXPECT_EQ(factory.registered_tags(), expected);
}

TEST(FactoryMethodTest, 生成に失敗するとnullptrが返り登録もされない)
{
  MemoryLoggerFactory factory;

  auto ng = factory.create("");
  EXPECT_EQ(ng, nullptr);
  EXPECT_TRUE(factory.registered_tags().empty());

  auto ok = factory.create("imu");
  ASSERT_NE(ok, nullptr);
  const std::vector<std::string> expected{"imu"};
  EXPECT_EQ(factory.registered_tags(), expected);
}

TEST(FactoryMethodTest, 生成物の所有権が呼び出し側に移る)
{
  int alive = 0;
  MemoryLoggerFactory factory{&alive};

  EXPECT_EQ(alive, 0);
  {
    auto logger = factory.create("motor");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(alive, 1);
  }
  // スコープを抜けたら破棄される。ファクトリは所有していない
  EXPECT_EQ(alive, 0);

  // ファクトリが死んでも、登録済みのタグが残るだけで Logger は復活しない
  EXPECT_EQ(factory.registered_tags().size(), 1u);
}

TEST(FactoryMethodTest, ムーブで所有権がさらに移る)
{
  int alive = 0;
  MemoryLoggerFactory factory{&alive};

  std::unique_ptr<Logger> outer;
  {
    auto inner = factory.create("imu");
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(alive, 1);

    outer = std::move(inner);
    EXPECT_EQ(inner, nullptr);   // ムーブ後は必ず nullptr
  }
  // 内側のスコープを抜けても、所有者は outer なのでまだ生きている
  EXPECT_EQ(alive, 1);
  ASSERT_NE(outer, nullptr);
  EXPECT_EQ(outer->tag(), "imu");

  outer.reset();
  EXPECT_EQ(alive, 0);
}

TEST(FactoryMethodTest, ファクトリは複数の生成物を所有しない)
{
  int alive = 0;
  {
    MemoryLoggerFactory factory{&alive};
    auto a = factory.create("a");
    auto b = factory.create("b");
    auto c = factory.create("c");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(alive, 3);
  }
  // ファクトリと 3 つの unique_ptr が同時に消える。二重解放は起きない
  EXPECT_EQ(alive, 0);
}
