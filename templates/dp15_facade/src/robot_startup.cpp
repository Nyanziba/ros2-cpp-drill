// I AM NOT DONE
//
// 結城本 第15章 Facade を C++ で書きます。
//
// この課題では 2 通り作ります。
//   1. 名前空間 + 自由関数版（robot::start_once）— 後始末を自分で呼びます
//   2. RAII クラス版（robot::RobotSession）— 後始末をデストラクタに任せます
// 同じ条件を与えたら、ログが 1 文字も違わないこと。これがテストの主眼です。
//
// 内部のサブシステムは下の無名名前空間にあります。ヘッダには出しません。
// 「見せる面を減らす」のが Facade の本質です。

#include "drill/robot_startup.hpp"

#include <string>
#include <utility>
#include <vector>

namespace robot
{

namespace
{

/// ログ出力。log が nullptr のときは何もしません。（実装済み。書き換え不要）
void append(std::vector<std::string> * log, const char * entry)
{
  if (log != nullptr) {
    log->push_back(entry);
  }
}

// --- ここから下が「Facade が隠す相手」です ----------------------------------
// ヘッダに書いていないので、利用者からは名前すら見えません。

/// 電源投入。config.battery_mv が kMinBatteryMv 以上なら成功。
bool power_on(const StartupConfig & config, std::vector<std::string> * log)
{
  // TODO: 成功なら append(log, "power_on") して true、
  //       失敗なら append(log, "power_on_failed") して false を返してください。
  (void)config;
  (void)log;
  return false;
}

/// 電源遮断。power_on() が成功したときだけ呼ばれます。
void power_off(std::vector<std::string> * log)
{
  // TODO: "power_off" をログに足してください。
  (void)log;
}

/// センサ初期化。config.sensor_present が true なら成功。
bool sensor_init(const StartupConfig & config, std::vector<std::string> * log)
{
  // TODO: "sensor_init" / "sensor_init_failed"
  (void)config;
  (void)log;
  return false;
}

/// センサ停止。
void sensor_deinit(std::vector<std::string> * log)
{
  // TODO: "sensor_deinit"
  (void)log;
}

/// キャリブレーション。config.calibration_ok が true なら成功。
bool calibrate(const StartupConfig & config, std::vector<std::string> * log)
{
  // TODO: "calibrate" / "calibrate_failed"
  (void)config;
  (void)log;
  return false;
}

/// キャリブレーション結果の破棄。
void calibration_clear(std::vector<std::string> * log)
{
  // TODO: "calibration_clear"
  (void)log;
}

/// 通信確立。config.link_ok が true なら成功。
bool link_up(const StartupConfig & config, std::vector<std::string> * log)
{
  // TODO: "link_up" / "link_up_failed"
  (void)config;
  (void)log;
  return false;
}

/// 通信切断。
void link_down(std::vector<std::string> * log)
{
  // TODO: "link_down"
  (void)log;
}

/// 完了した段の数だけ、初期化と**逆順**で後始末する。
///   completed_stages == 4 → link_down → calibration_clear → sensor_deinit → power_off
///   completed_stages == 2 → sensor_deinit → power_off
///   completed_stages == 0 → 何もしない
void teardown(int completed_stages, std::vector<std::string> * log)
{
  // TODO: 上のとおりに実装してください。
  //       段が増えたときに直す場所を 1 箇所にするため、後始末はここにだけ書きます。
  (void)completed_stages;
  (void)log;
  (void)&append;
  (void)&link_down;
  (void)&calibration_clear;
  (void)&sensor_deinit;
  (void)&power_off;
}

}  // namespace

StartupResult start_once(const StartupConfig & config, std::vector<std::string> * log)
{
  // TODO: 名前空間 + 自由関数版の Facade です。クラスにしてはいけません。
  //
  //   1. power_on → sensor_init → calibrate → link_up の順に呼ぶ
  //   2. どこかで false が返ったら、そこで打ち切り、
  //      teardown(それまでに成功した段数, log) を呼んでから
  //      StartupResult{false, 失敗した段} を返す
  //   3. 全部成功したら teardown(4, log) を呼んでから
  //      StartupResult{true, StartupStage::kPower} を返す
  //
  // 「成功しても後始末する」のは、この関数が「1 回起動して、その場で止める」窓口だからです。
  // 起動したまま使い続けたいなら RobotSession の方を使います。
  //
  // 注意: return が 5 通りあります。**そのすべてで teardown() を呼ぶ**必要があります。
  //       呼び忘れても誰も怒ってくれません。それが RAII 版（下）との差です。
  (void)config;
  (void)log;
  (void)&teardown;
  (void)&power_on;
  (void)&sensor_init;
  (void)&calibrate;
  (void)&link_up;
  return StartupResult{false, StartupStage::kPower};
}

RobotSession::RobotSession(StartupConfig config, std::vector<std::string> * log)
: config_(std::move(config)),
  log_(log),
  completed_stages_(0),
  ready_(false),
  failed_stage_(StartupStage::kPower)
{
  // TODO: 起動シーケンスをここで走らせます。
  //   - 段が 1 つ成功するたびに completed_stages_ を 1 増やす
  //   - 失敗したら failed_stage_ にその段を入れて、そこで打ち切る（return してよい）
  //   - 4 段すべて成功したら ready_ = true
  //
  // **後始末をここに書いてはいけません。** デストラクタの仕事です。
  // コンストラクタの途中で return しても、このオブジェクトは構築済みなので
  // デストラクタが必ず走り、completed_stages_ の分だけ巻き戻されます。
  //
  // 例外は投げません（マイコンでは -fno-exceptions が普通）。
  // 呼び出し側は is_ready() で確かめます。
  (void)config_;
}

RobotSession::~RobotSession()
{
  // TODO: teardown(completed_stages_, log_) を呼ぶだけです。
  //
  // ムーブ元のセッションは completed_stages_ が 0 になっているので、
  // 素直に書けば「後始末が 2 回走る」事故は起きません。
}

RobotSession::RobotSession(RobotSession && other) noexcept
: config_(std::move(other.config_)),
  log_(other.log_),
  completed_stages_(other.completed_stages_),
  ready_(other.ready_),
  failed_stage_(other.failed_stage_)
{
  // TODO: ムーブ元を「何も持っていない」状態にしてください。
  //       completed_stages_ を 0 に、ready_ を false に、log_ を nullptr に。
  //       これを忘れると後始末が 2 回走ります。
  (void)other;
}

bool RobotSession::drive(int duty)
{
  // TODO: ready_ が false なら何もせず false。
  //       true なら log_ に "drive:" + std::to_string(duty) を足して true。
  //       （append() は const char * を取るので、ここは log_->push_back() を直接使います。
  //         log_ が nullptr のこともあるので気をつけてください）
  (void)duty;
  return false;
}

}  // namespace robot
