// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <vector>

#include "drill/sensor_reader.hpp"

namespace
{

const std::vector<std::string> kFullSteps = {"initialize", "fetch_raw", "convert", "validate"};
const std::vector<std::string> kStepsWithoutInitialize = {"fetch_raw", "convert", "validate"};

}  // namespace

TEST(TemplateMethodTest, 骨格が決められた順番で各段を呼ぶ)
{
  EncoderReader encoder({0}, 1000.0);
  const auto value = encoder.read_once();

  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(encoder.call_log(), kFullSteps);
}

TEST(TemplateMethodTest, 初期化は初回の一度だけ走る)
{
  EncoderReader encoder({0, 250}, 1000.0);
  encoder.read_once();
  encoder.read_once();

  std::vector<std::string> expected = kFullSteps;
  expected.insert(expected.end(), kStepsWithoutInitialize.begin(), kStepsWithoutInitialize.end());

  EXPECT_EQ(encoder.call_log(), expected);
  EXPECT_TRUE(encoder.is_initialized());
}

TEST(TemplateMethodTest, 読み取り前は初期化されていない)
{
  EncoderReader encoder({0}, 1000.0);
  EXPECT_FALSE(encoder.is_initialized());
  EXPECT_TRUE(encoder.call_log().empty());
}

TEST(TemplateMethodTest, エンコーダがカウント値を角度に変換する)
{
  EncoderReader encoder({0, 250, 1000}, 1000.0);

  const auto first = encoder.read_once();
  const auto second = encoder.read_once();
  const auto third = encoder.read_once();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  ASSERT_TRUE(third.has_value());
  EXPECT_DOUBLE_EQ(*first, 0.0);
  EXPECT_DOUBLE_EQ(*second, 90.0);
  EXPECT_DOUBLE_EQ(*third, 360.0);
}

TEST(TemplateMethodTest, サーミスタがAD値を温度に変換する)
{
  ThermistorReader thermistor({200, 400});

  const auto first = thermistor.read_once();
  const auto second = thermistor.read_once();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  EXPECT_NEAR(*first, 0.0, 1e-9);
  EXPECT_NEAR(*second, 20.0, 1e-9);
}

TEST(TemplateMethodTest, サーミスタは範囲外の温度をnulloptで返す)
{
  ThermistorReader thermistor({2000});   // 180 degC。上限を超えている

  const auto value = thermistor.read_once();

  EXPECT_FALSE(value.has_value());
  // 検証に落ちても、そこまでの手順は最後まで踏んでいること
  EXPECT_EQ(thermistor.call_log(), kFullSteps);
}

TEST(TemplateMethodTest, 差し替えたvalidateが基底ではなく派生の実装で呼ばれる)
{
  // 同じ 2000 という生値でも、エンコーダ側は範囲判定を持たないので値が返る
  EncoderReader encoder({2000}, 1000.0);
  const auto encoder_value = encoder.read_once();
  ASSERT_TRUE(encoder_value.has_value());
  EXPECT_DOUBLE_EQ(*encoder_value, 720.0);

  ThermistorReader thermistor({2000});
  EXPECT_FALSE(thermistor.read_once().has_value());
}

TEST(TemplateMethodTest, 基底クラスのポインタ経由でも同じ手順が走る)
{
  ThermistorReader thermistor({400});
  SensorReader & reader = thermistor;

  const auto value = reader.read_once();

  ASSERT_TRUE(value.has_value());
  EXPECT_NEAR(*value, 20.0, 1e-9);
  EXPECT_EQ(reader.call_log(), kFullSteps);
}
