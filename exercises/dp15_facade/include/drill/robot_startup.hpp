// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <string>
#include <vector>

namespace robot
{

/// 起動シーケンスの 4 段。失敗した段を報告するために使います。
enum class StartupStage
{
  kPower,         ///< 電源投入
  kSensor,        ///< センサ初期化
  kCalibration,   ///< キャリブレーション
  kLink,          ///< 通信確立
};

/// 起動条件。テストから各段を意図的に失敗させるために外から与えます。
/// 実機では電圧はセンサから読み、present / ok は自己診断の結果になります。
struct StartupConfig
{
  int battery_mv = 12000;       ///< kMinBatteryMv 未満なら電源投入に失敗
  bool sensor_present = true;   ///< false ならセンサ初期化に失敗
  bool calibration_ok = true;   ///< false ならキャリブレーションに失敗
  bool link_ok = true;          ///< false なら通信確立に失敗
};

/// 起動できる最低電圧 [mV]。
constexpr int kMinBatteryMv = 11000;

/// 起動の結果。ok が true のとき failed_stage は読みません。
struct StartupResult
{
  bool ok = false;
  StartupStage failed_stage = StartupStage::kPower;
};

/// 「1 回起動して、その場で止める」窓口。
///
/// 結城本の PageMaker（static メソッドだけのクラス）に相当しますが、
/// C++ では **クラスにしません**。状態を持たないなら名前空間 + 自由関数で足ります。
///
/// log が非 nullptr なら、内部で走った手順の名前を順に push_back します。
/// 後始末まで含めて必ず走り、戻ったときには何も初期化されていません。
StartupResult start_once(const StartupConfig & config, std::vector<std::string> * log = nullptr);

/// 「初期化して、使って、後始末する」窓口。**こちらが C++ 版 Facade の本命です。**
///
/// コンストラクタが起動シーケンスを走らせ、デストラクタが後始末を逆順で走らせます。
/// 途中の段で失敗したら、それ以降は走らず、**すでに終わった段だけ**が巻き戻されます。
///
/// 内部のサブシステム（電源・センサ・キャリブレータ・通信）はこのヘッダに出てきません。
/// 見せる面を減らすのが Facade の本質です（第9章 Bridge / Pimpl と目的が近い）。
class RobotSession
{
public:
  /// 起動シーケンスを走らせる。失敗しても例外は投げません。is_ready() で確かめます。
  explicit RobotSession(StartupConfig config, std::vector<std::string> * log = nullptr);

  /// 成功した段だけを逆順で後始末する。
  ~RobotSession();

  // セッションは「起動済みのハードウェア 1 台」を表します。コピーに意味はありません。
  RobotSession(const RobotSession &) = delete;
  RobotSession & operator=(const RobotSession &) = delete;

  // 関数から返せるようにムーブ構築だけ許します。
  // ムーブ代入は「代入先が持っている起動状態をいつ落とすか」を決める必要があり、
  // この課題では要らないので禁止します。
  RobotSession(RobotSession && other) noexcept;
  RobotSession & operator=(RobotSession &&) = delete;

  /// 4 段すべてが成功していれば true。
  bool is_ready() const { return ready_; }

  /// is_ready() が false のときだけ意味を持ちます。
  StartupStage failed_stage() const { return failed_stage_; }

  /// 起動済みのときだけ走る操作。ログに "drive:<duty>" を残します。
  /// 起動できていなければ何もせず false。
  bool drive(int duty);

private:
  StartupConfig config_;
  std::vector<std::string> * log_;
  int completed_stages_;   ///< 成功して完了した段の数（0〜4）。後始末の範囲になる
  bool ready_;
  StartupStage failed_stage_;
};

}  // namespace robot
