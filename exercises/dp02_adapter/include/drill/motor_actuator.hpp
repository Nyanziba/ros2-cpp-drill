// このファイルは編集しません（インタフェースの提示）。
//
// 部内共通のアクチュエータインタフェース（Target 役）。
// 上位の制御コードはこの型だけを見ます。生ドライバの存在を知りません。
#pragma once

/// 1 [rad/s] あたりのパルス指令値。
inline constexpr double PULSES_PER_RAD_PER_SEC = 100.0;

/// 1 [rad] あたりのエンコーダカウント。
inline constexpr double COUNTS_PER_RAD = 200.0;

/// 部内共通のモータインタフェース。単位は SI（rad/s, rad）。
class MotorActuator
{
public:
  // 基底ポインタで delete されるので仮想デストラクタは必須。
  virtual ~MotorActuator() = default;

  /// 目標角速度 [rad/s] を与える。
  virtual void set_velocity(double rad_per_sec) = 0;

  /// 停止する。
  virtual void stop() = 0;

  /// 現在角度 [rad] を返す。
  virtual double position_rad() const = 0;
};
