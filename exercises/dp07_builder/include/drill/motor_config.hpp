// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstdint>
#include <optional>
#include <string>

// ---------------------------------------------------------------------------
// 実務でよく見る Builder（メソッドチェーンで引数を埋める）
//
// 注意: これは結城本 第7章の Builder（Director + Builder）とは**別物**です。
//       同じ「Builder」という名前が付いているだけで、解いている問題が違います。
//         - 結城本の Builder : 手順（Director）と部品（Builder）を分ける
//         - こちらの Builder : C++ に名前付き引数が無いのを埋める
//       詳しくは docs/patterns/07_Builder.md 7.1 を読んでください。
// ---------------------------------------------------------------------------

/// モータ 1 台ぶんの設定。作られたあとは書き換えない前提の値オブジェクトです。
///
/// この型を直接コンストラクタで作ると、呼び出しはこうなります。
///   MotorConfig cfg{3, "drive_left", 0.8, 12.0, 8192, true, false};
/// 何が true で何が false なのか、呼び出し側からは読めません。
/// **C++17 には名前付き引数も designated initializer も無い**ので、
/// これを読めるようにするのが MotorConfigBuilder の仕事です。
struct MotorConfig
{
  std::uint8_t motor_id = 0;               ///< 必須。既定値は無い
  std::string name = "unnamed";            ///< ログに出す名前
  double max_duty = 1.0;                   ///< 0.0〜1.0
  double current_limit_ampere = 5.0;       ///< 電流制限 [A]
  std::uint32_t encoder_counts_per_rev = 4096;
  bool invert_direction = false;
  bool brake_on_stop = true;
};

/// MotorConfig を組み立てる Builder。
///
/// 使いかた:
///   const auto config = MotorConfigBuilder{}
///                         .motor_id(3)
///                         .name("drive_left")
///                         .max_duty(0.8)
///                         .build();
///   if (!config) { /* 必須項目が足りない */ }
///
/// 設計上の約束:
///   - セッタは **MotorConfigBuilder &（参照）** を返します。
///     値で返すとチェーンのたびに Builder ごとコピーされます。
///   - build() は **std::optional** を返します。例外を投げません。
///     マイコンでは -fno-exceptions が普通なので、例外に頼らない形に揃えています。
///   - motor_id だけが必須です。設定せずに build() すると std::nullopt が返ります。
class MotorConfigBuilder
{
public:
  MotorConfigBuilder & motor_id(std::uint8_t id);
  MotorConfigBuilder & name(std::string motor_name);
  MotorConfigBuilder & max_duty(double duty);
  MotorConfigBuilder & current_limit_ampere(double ampere);
  MotorConfigBuilder & encoder_counts_per_rev(std::uint32_t counts);
  MotorConfigBuilder & invert_direction(bool inverted);
  MotorConfigBuilder & brake_on_stop(bool brake);

  /// 左辺値（名前の付いた Builder）から呼ばれる版。
  /// Builder はこのあとも使える可能性があるので、**コピーして**返します。
  std::optional<MotorConfig> build() const &;

  /// 右辺値（一時オブジェクト、または std::move された Builder）から呼ばれる版。
  /// もう誰も見ないので、**ムーブして**返します。std::string のコピーが 1 回減ります。
  ///
  /// この 2 つを分けられるのが参照修飾子（ref-qualifier）です。
  /// Java には無い C++ 固有の機能です。
  std::optional<MotorConfig> build() &&;

  /// 組み立て途中の中身を覗く。テストとデバッグ用で、実務では普通は要りません。
  /// （テストが「ムーブされたか / コピーされたか」を確かめるために使います）
  const MotorConfig & peek() const { return config_; }

private:
  MotorConfig config_{};
  bool has_motor_id_ = false;
};

// ---------------------------------------------------------------------------
// マイコン向け: ヒープを一切使わない constexpr Builder
//
// ここは **実装済みの見本** です。課題ではありません。読んでください。
// 全部 constexpr なので、下のように書くとコンパイル時に組み立てが終わり、
// 出来上がった値がそのまま ROM（.rodata）に置かれます。実行時コストはゼロです。
//
//   constexpr ControlLimits kDriveLimits =
//     ControlLimitsBuilder{}.max_velocity(20.0F).max_accel(80.0F).build();
// ---------------------------------------------------------------------------

/// 制御の上限値。std::string を持たないので、確保が一切走りません。
struct ControlLimits
{
  float max_velocity_rad_per_sec = 10.0F;
  float max_accel_rad_per_sec2 = 50.0F;
  float max_current_ampere = 5.0F;
};

/// ControlLimits を組み立てる constexpr Builder。
///
/// C++14 以降、constexpr 関数の中でメンバを書き換えられます。
/// なので「チェーンで埋めていく」書き方がそのままコンパイル時に走ります。
class ControlLimitsBuilder
{
public:
  constexpr ControlLimitsBuilder & max_velocity(float value)
  {
    limits_.max_velocity_rad_per_sec = value;
    return *this;
  }

  constexpr ControlLimitsBuilder & max_accel(float value)
  {
    limits_.max_accel_rad_per_sec2 = value;
    return *this;
  }

  constexpr ControlLimitsBuilder & max_current(float value)
  {
    limits_.max_current_ampere = value;
    return *this;
  }

  constexpr ControlLimits build() const { return limits_; }

private:
  ControlLimits limits_{};
};
