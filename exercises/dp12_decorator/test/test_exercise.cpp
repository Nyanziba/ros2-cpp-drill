// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "drill/log_sink.hpp"
#include "drill_i18n.hpp"
#include "drill_i18n.hpp"

namespace
{

const char * const kStamp = "12:00:00.000";

}  // namespace

TEST(DecoratorTest, JoinTagJoinsTagAndBodyWithSpace)
{
  EXPECT_EQ(join_tag("[INFO]", "moving"), "[INFO] moving");
}

TEST(DecoratorTest, PlainMessageIsReturnedUnchanged)
{
  const PlainMessage sink;
  EXPECT_EQ(sink.format("moving"), "moving");
}

TEST(DecoratorTest, LevelTagIsPrepended)
{
  const LevelTag sink(std::make_unique<PlainMessage>(), "INFO");
  EXPECT_EQ(sink.format("moving"), "[INFO] moving");
}

TEST(DecoratorTest, LocationTagListsFileNameAndLineNumber)
{
  const SourceTag sink(std::make_unique<PlainMessage>(), "sensor.cpp", 42);
  EXPECT_EQ(sink.format("moving"), "sensor.cpp:42 moving");
}

TEST(DecoratorTest, WrappingOrderChangesOutput)
{
  // レベルで包んでから時刻で包む → 時刻が外側
  const TimestampTag outer_time(
    std::make_unique<LevelTag>(std::make_unique<PlainMessage>(), "INFO"), kStamp);

  // 時刻で包んでからレベルで包む → レベルが外側
  const LevelTag outer_level(
    std::make_unique<TimestampTag>(std::make_unique<PlainMessage>(), kStamp), "INFO");

  EXPECT_EQ(outer_time.format("moving"), "12:00:00.000 [INFO] moving");
  EXPECT_EQ(outer_level.format("moving"), "[INFO] 12:00:00.000 moving");
  EXPECT_NE(outer_time.format("moving"), outer_level.format("moving"));
}

TEST(DecoratorTest, CanWrapMultipleLayers)
{
  auto sink = with_level(
    with_timestamp(with_source(plain(), "sensor.cpp", 42), kStamp), "WARN");
  ASSERT_NE(sink, nullptr) << drill::localized("ヘルパ関数が nullptr を返しています", "The helper function returned nullptr");

  EXPECT_EQ(sink->format("timeout"), "[WARN] 12:00:00.000 sensor.cpp:42 timeout");
}

TEST(DecoratorTest, HelperAndMakeUniqueProduceSameResult)
{
  auto by_helper = with_level(with_timestamp(plain(), kStamp), "INFO");
  ASSERT_NE(by_helper, nullptr);

  const std::unique_ptr<LogSink> by_make_unique = std::make_unique<LevelTag>(
    std::make_unique<TimestampTag>(std::make_unique<PlainMessage>(), kStamp), "INFO");

  EXPECT_EQ(by_helper->format("moving"), by_make_unique->format("moving"));
}

TEST(DecoratorTest, DestroyingOutermostDestroysAllInner)
{
  DestructionLog::clear();

  {
    auto sink = with_level(with_timestamp(plain(), kStamp), "INFO");
    ASSERT_NE(sink, nullptr);
    EXPECT_TRUE(DestructionLog::entries().empty()) << drill::localized("組み立てただけで何かが壊れています", "Something was destroyed just by building");
  }

  // 外側から順に、内側まで到達すること。
  const std::vector<std::string> expected = {"LevelTag", "TimestampTag", "PlainMessage"};
  EXPECT_EQ(DestructionLog::entries(), expected)
    << drill::localized(
         "入れ子の内側が解放されていません。SinkDecorator が中身を unique_ptr で"
         "所有しているか、各デストラクタが記録しているかを確認してください",
         "The inner part of the nesting was not released. Check that SinkDecorator owns its content with unique_ptr, "
         "and that each destructor records its destruction");

  DestructionLog::clear();
}

TEST(DecoratorTest, TemplateVersionReturnsSameStringAsUniquePtrVersion)
{
  const StaticLevelTag<StaticTimestampTag<StaticPlainMessage>> static_sink(
    StaticTimestampTag<StaticPlainMessage>(StaticPlainMessage{}, kStamp), "INFO");

  auto dynamic_sink = with_level(with_timestamp(plain(), kStamp), "INFO");
  ASSERT_NE(dynamic_sink, nullptr);

  EXPECT_EQ(static_sink.format("moving"), "[INFO] 12:00:00.000 moving");
  EXPECT_EQ(static_sink.format("moving"), dynamic_sink->format("moving"));
}

TEST(DecoratorTest, TemplateVersionHasNoVirtualFunctions)
{
  using StaticSink = StaticLevelTag<StaticTimestampTag<StaticPlainMessage>>;

  static_assert(!std::is_polymorphic_v<StaticPlainMessage>, "vtable があってはいけません / There must be no vtable");
  static_assert(!std::is_polymorphic_v<StaticSink>, "vtable があってはいけません / There must be no vtable");
  static_assert(std::is_polymorphic_v<LevelTag>, "unique_ptr 版はこちらが多態です / The unique_ptr version is the polymorphic one");

  // vtable ポインタが無いので、メンバの合計より大きくなりません。
  EXPECT_LT(sizeof(StaticPlainMessage), sizeof(LevelTag));

  const StaticSink sink(
    StaticTimestampTag<StaticPlainMessage>(StaticPlainMessage{}, kStamp), "INFO");
  EXPECT_EQ(sink.format("moving"), "[INFO] 12:00:00.000 moving");
}
