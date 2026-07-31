// この課題はここが唯一の編集対象です。
//
// 他の課題と逆で、あなたが書くのは「実装」ではなく「テスト」です。
// このテストは 2 つの実装に対して走ります。
//   - 正しい実装 … 通らなければ不合格
//   - バグ入り実装 … 落ちなければ（＝バグを見逃していれば）不合格
// 詳しくは README を読んでください。

// I AM NOT DONE

#include <gtest/gtest.h>

#include "drill/velocity_limiter.hpp"

// 例: 制限にかからない普通のケース。
// target までの変化量も、結果の大きさも max_delta / max_speed の範囲内なので、
// 加速度制限にも速度制限にも引っかからず target がそのまま返るはずです。
//
// ただし、このテストだけではバグを検出できません（正常系しか見ていないため）。
// 下の TODO を埋めて、観点を増やしてください。
TEST(VelocityLimiterTest, 制限にかからない場合はtargetがそのまま返る)
{
  const double result = limit_velocity(/*target=*/1.0, /*previous=*/0.0,
    /*max_speed=*/10.0, /*max_delta=*/10.0);
  EXPECT_DOUBLE_EQ(result, 1.0);
}

// TODO: 速度制限が「負の側」にも効いているか確かめるテストを書くこと。
//       （previous を先に大きく負の値へ動かしてから limit_velocity を呼ぶなど、
//         加速度制限には引っかからないように previous を選ぶとよい）

// TODO: 加速度制限してから速度制限、の「順番」で適用されているか確かめる
//       テストを書くこと。一気に大きく動かそうとしたときにどうなるかを考える。

// TODO: max_delta や max_speed に 0 や負の値が渡された場合の扱いを確かめる
//       テストを書くこと。仕様上「負が来たら 0 として扱う」とされている。

// TODO: 変化量がちょうど max_delta と等しい、または結果がちょうど max_speed と
//       等しい、といった境界値でどうなるか確かめるテストを書くこと。
