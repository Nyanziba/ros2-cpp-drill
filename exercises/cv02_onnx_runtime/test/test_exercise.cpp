// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "drill/color_session.hpp"
#include "drill_i18n.hpp"

#ifndef DRILL_MODEL_PATH
#error "DRILL_MODEL_PATH is not set"
#endif

namespace
{

std::vector<std::uint8_t> solid(int width, int height, std::uint8_t b, std::uint8_t g, std::uint8_t r)
{
  std::vector<std::uint8_t> data(static_cast<std::size_t>(width * height * 3));
  for (int i = 0; i < width * height; ++i) {
    data[static_cast<std::size_t>(i * 3)] = b;
    data[static_cast<std::size_t>(i * 3 + 1)] = g;
    data[static_cast<std::size_t>(i * 3 + 2)] = r;
  }
  return data;
}

int classify(const std::vector<std::uint8_t> & data, int width, int height)
{
  return drill::classify_marker_onnx(DRILL_MODEL_PATH, data.data(), width, height);
}

}  // namespace

TEST(OnnxColor, RejectsEmpty)
{
  EXPECT_EQ(drill::classify_marker_onnx(DRILL_MODEL_PATH, nullptr, 2, 2), -1);
  const auto data = solid(2, 2, 0, 0, 0);
  EXPECT_EQ(drill::classify_marker_onnx(DRILL_MODEL_PATH, data.data(), 0, 2), -1);
}

TEST(OnnxColor, ClassifiesSolidColors)
{
  EXPECT_EQ(classify(solid(4, 4, 0, 0, 0), 4, 4), 0) << drill::localized(
    "黒はクラス 0（マーカーなし）です。", "Black is class 0 (no marker).");
  EXPECT_EQ(classify(solid(4, 4, 0, 0, 255), 4, 4), 1) << drill::localized(
    "純赤はクラス 1 です。", "Pure red is class 1.");
  EXPECT_EQ(classify(solid(4, 4, 255, 0, 0), 4, 4), 2) << drill::localized(
    "純青はクラス 2 です。", "Pure blue is class 2.");
  EXPECT_EQ(classify(solid(4, 4, 0, 255, 0), 4, 4), 0) << drill::localized(
    "緑の床はマーカーではありません。", "A green floor is not a marker.");
}

TEST(OnnxColor, HalfRedIsStillRed)
{
  auto data = solid(2, 1, 0, 0, 0);
  data[2] = 255;
  EXPECT_EQ(classify(data, 2, 1), 1);
}
