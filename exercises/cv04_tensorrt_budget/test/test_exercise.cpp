// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "drill/precision_budget.hpp"
#include "drill_i18n.hpp"

TEST(PrecisionBudget, ChoosesInt8OnlyWithEnoughImages)
{
  EXPECT_EQ(drill::choose_precision(true, true, 32), drill::Precision::Int8);
  EXPECT_EQ(drill::choose_precision(true, true, 31), drill::Precision::Fp16);
  EXPECT_EQ(drill::choose_precision(false, true, 100), drill::Precision::Int8);
  EXPECT_EQ(drill::choose_precision(false, false, 100), drill::Precision::Fp32);
  EXPECT_EQ(drill::choose_precision(true, false, 0), drill::Precision::Fp16);
}

TEST(PrecisionBudget, FrameBudgetLeavesMargin)
{
  EXPECT_TRUE(drill::meets_frame_budget(23.0, 30.0, 0.30));
  EXPECT_FALSE(drill::meets_frame_budget(24.0, 30.0, 0.30)) << drill::localized(
    "30 fps の 70% は約 23.3 ms です。", "70% of a 30 fps frame is about 23.3 ms.");
  EXPECT_FALSE(drill::meets_frame_budget(1.0, 0.0, 0.30));
  EXPECT_NEAR(drill::speedup_ratio(8.0, 2.0), 4.0, 1e-9);
  EXPECT_EQ(drill::speedup_ratio(8.0, 0.0), 0.0);
}

TEST(PrecisionBudget, CalibrationSetSpreadsHistograms)
{
  const std::vector<drill::HistogramSample> samples{
    {"lab", {0, 0, 0, 0, 0, 0, 0, 0}},
    {"near", {5, 0, 0, 0, 0, 0, 0, 0}},
    {"side", {0, 0, 0, 0, 0, 0, 0, 9}},
    {"field", {5, 0, 0, 0, 0, 0, 0, 9}},
  };
  EXPECT_EQ(drill::pick_calibration_set(samples, 2), (std::vector<std::string>{"lab", "field"}));
  EXPECT_EQ(
    drill::pick_calibration_set(samples, 3),
    (std::vector<std::string>{"lab", "field", "near"}));
  EXPECT_TRUE(drill::pick_calibration_set(samples, 0).empty());
}

TEST(PrecisionBudget, ParsesTrtexecMean)
{
  double mean = 0.0;
  const std::string line =
    "GPU Compute Time: min = 1.20 ms, max = 3.40 ms, mean = 1.80 ms, median = 1.70 ms";
  ASSERT_TRUE(drill::parse_trtexec_mean_ms(line, mean));
  EXPECT_NEAR(mean, 1.80, 1e-9);
  EXPECT_FALSE(drill::parse_trtexec_mean_ms("no timing here", mean));
}

TEST(PrecisionBudget, TensorRtEngineBuildIsSkippedWithoutGpu)
{
#if !defined(DRILL_HAS_TENSORRT)
  GTEST_SKIP() << drill::localized(
    "TensorRT SDK と NVIDIA GPU が無いのでエンジン構築はスキップします。"
    " CI では精度の選択、フレーム予算、キャリブレーション集合、trtexec の mean だけを採点します。",
    "Skipped: building an engine needs the TensorRT SDK and an NVIDIA GPU. "
    "CI grades precision choice, the frame budget, the calibration set, and the trtexec mean.");
#else
  FAIL() << "DRILL_HAS_TENSORRT is set, but this drill does not build an engine.";
#endif
}
