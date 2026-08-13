// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drill/motor_config.hpp"
#include "drill/telemetry_builder.hpp"

namespace
{

/// 呼ばれた順番だけを記録する Builder。
/// 「Director が手順を持っている」ことを直接確かめるために使います。
class RecordingTelemetryBuilder : public TelemetryBuilder
{
public:
  void make_header(const std::string & title) override
  {
    calls_.push_back("header:" + title);
  }

  void make_field(const std::string & key, double value) override
  {
    calls_.push_back("field:" + key + "=" + format_number(value));
  }

  void make_footer() override { calls_.push_back("footer"); }

  const std::vector<std::string> & calls() const { return calls_; }

private:
  std::vector<std::string> calls_;
};

/// SSO（小さい文字列の最適化）に収まらない長さ。
/// これより短いと、ムーブでもコピーでもバッファのアドレスが変わってしまい、
/// 「ムーブされたか」を観測できません。
const std::string kLongName(64, 'x');

}  // namespace

// --- 型の性質をコンパイル時に確かめる（テストではなく静的検査） ------------

static_assert(
  std::is_same_v<
    decltype(std::declval<MotorConfigBuilder &>().motor_id(std::uint8_t{0})),
    MotorConfigBuilder &>,
  "セッタは MotorConfigBuilder & を返してください。値で返すとチェーンのたびにコピーされます");

static_assert(
  std::is_same_v<
    decltype(std::declval<MotorConfigBuilder &>().name(std::string{})),
    MotorConfigBuilder &>,
  "セッタは MotorConfigBuilder & を返してください");

/// マイコン向けの constexpr Builder。組み立てがコンパイル時に終わることの証明。
constexpr ControlLimits kDriveLimits =
  ControlLimitsBuilder{}.max_velocity(20.0F).max_accel(80.0F).build();

static_assert(kDriveLimits.max_velocity_rad_per_sec == 20.0F, "constexpr で組み立てられていません");
static_assert(kDriveLimits.max_accel_rad_per_sec2 == 80.0F, "constexpr で組み立てられていません");
static_assert(kDriveLimits.max_current_ampere == 5.0F, "設定していない項目は既定値のはずです");

// --- 結城本の形（Director + Builder） --------------------------------------

TEST(BuilderTest, 同じDirectorがCSVを組み立てる)
{
  CsvTelemetryBuilder builder;
  TelemetryDirector director{builder};
  director.construct();

  const std::string expected =
    "# robot telemetry\n"
    "battery_voltage,12.5\n"
    "motor_current,3.25\n"
    "cpu_temperature,41\n"
    "# end\n";
  EXPECT_EQ(builder.result(), expected);
}

TEST(BuilderTest, 同じDirectorがJSONを組み立てる)
{
  JsonTelemetryBuilder builder;
  TelemetryDirector director{builder};
  director.construct();

  const std::string expected =
    "{\n"
    "  \"title\": \"robot telemetry\",\n"
    "  \"battery_voltage\": 12.5,\n"
    "  \"motor_current\": 3.25,\n"
    "  \"cpu_temperature\": 41\n"
    "}\n";
  EXPECT_EQ(builder.result(), expected);
}

TEST(BuilderTest, Directorが手順を持ちBuilderは呼ばれる順を知らない)
{
  RecordingTelemetryBuilder builder;
  TelemetryDirector director{builder};
  director.construct();

  const std::vector<std::string> expected = {
    "header:robot telemetry",
    "field:battery_voltage=12.5",
    "field:motor_current=3.25",
    "field:cpu_temperature=41",
    "footer"};
  EXPECT_EQ(builder.calls(), expected);
}

// --- 実務の形（メソッドチェーン） ------------------------------------------

TEST(MotorConfigBuilderTest, チェーンで設定した値がすべて反映される)
{
  const auto config = MotorConfigBuilder{}
                        .motor_id(3)
                        .name("drive_left")
                        .max_duty(0.8)
                        .current_limit_ampere(12.0)
                        .encoder_counts_per_rev(8192)
                        .invert_direction(true)
                        .brake_on_stop(false)
                        .build();

  ASSERT_TRUE(config.has_value()) << "必須項目を埋めたのに build() が失敗しています";
  EXPECT_EQ(config->motor_id, 3);
  EXPECT_EQ(config->name, "drive_left");
  EXPECT_DOUBLE_EQ(config->max_duty, 0.8);
  EXPECT_DOUBLE_EQ(config->current_limit_ampere, 12.0);
  EXPECT_EQ(config->encoder_counts_per_rev, 8192U);
  EXPECT_TRUE(config->invert_direction);
  EXPECT_FALSE(config->brake_on_stop);
}

TEST(MotorConfigBuilderTest, 設定していない項目は既定値になる)
{
  const auto config = MotorConfigBuilder{}.motor_id(1).build();

  ASSERT_TRUE(config.has_value());
  EXPECT_EQ(config->motor_id, 1);
  EXPECT_EQ(config->name, "unnamed");
  EXPECT_DOUBLE_EQ(config->max_duty, 1.0);
  EXPECT_DOUBLE_EQ(config->current_limit_ampere, 5.0);
  EXPECT_EQ(config->encoder_counts_per_rev, 4096U);
  EXPECT_FALSE(config->invert_direction);
  EXPECT_TRUE(config->brake_on_stop);
}

TEST(MotorConfigBuilderTest, 必須項目が欠けたbuildはnulloptを返す)
{
  const auto missing = MotorConfigBuilder{}.name("drive_left").max_duty(0.5).build();
  EXPECT_FALSE(missing.has_value())
    << "motor_id を設定していないので std::nullopt を返してください";

  // 逆に、必須項目さえ埋まっていれば成功します。
  // 「いつも nullopt を返す」実装で通らないよう、対にして見ています。
  const auto filled = MotorConfigBuilder{}.name("drive_left").motor_id(9).build();
  ASSERT_TRUE(filled.has_value()) << "motor_id を設定したのに std::nullopt が返っています";
  EXPECT_EQ(filled->motor_id, 9);
}

TEST(MotorConfigBuilderTest, チェーンは同じBuilderの参照を返す)
{
  MotorConfigBuilder builder;

  // 値で返していると、ここで別のオブジェクトのアドレスが出ます。
  EXPECT_EQ(&builder, &builder.motor_id(7));
  EXPECT_EQ(&builder, &builder.name("arm"));
  EXPECT_EQ(&builder, &builder.max_duty(0.25));

  // アドレスが同じでも、値が捨てられていては意味がありません。
  const auto config = builder.build();
  ASSERT_TRUE(config.has_value());
  EXPECT_EQ(config->motor_id, 7);
  EXPECT_EQ(config->name, "arm");
  EXPECT_DOUBLE_EQ(config->max_duty, 0.25);
}

TEST(MotorConfigBuilderTest, 右辺値からのbuildはムーブになる)
{
  MotorConfigBuilder builder;
  builder.motor_id(2).name(kLongName);

  const char * before = builder.peek().name.data();
  const auto config = std::move(builder).build();

  ASSERT_TRUE(config.has_value());
  EXPECT_EQ(config->name, kLongName);
  EXPECT_EQ(config->name.data(), before)
    << "build() && ではバッファを移してください（std::move）";
}

TEST(MotorConfigBuilderTest, 左辺値からのbuildはコピーになりBuilderは壊れない)
{
  MotorConfigBuilder builder;
  builder.motor_id(2).name(kLongName);

  const char * before = builder.peek().name.data();
  const auto first = builder.build();

  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(first->name, kLongName);
  EXPECT_NE(first->name.data(), before)
    << "build() const & で中身を奪っています。ここはコピーです";

  // Builder は壊れていないので、もう一度使えます。
  const auto second = builder.build();
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(second->name, kLongName);
}
