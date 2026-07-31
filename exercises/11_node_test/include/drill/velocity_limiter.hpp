// このファイルは編集しません（インタフェースの提示）。
#pragma once

/// 速度指令を制限する。
///
/// 次の 2 つを順に適用した値を返す。
///  1. 加速度制限: previous から target への変化量の絶対値を max_delta 以内に収める
///  2. 速度制限:   結果の絶対値を max_speed 以内に収める
///
/// max_speed と max_delta は 0 以上を想定する（負が来たら 0 として扱う）。
double limit_velocity(double target, double previous, double max_speed, double max_delta);
