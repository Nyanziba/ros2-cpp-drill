// ROS 2 練習帳 共通テストヘルパ
//
// このファイルは編集しません。各課題のテストから include されます。
#pragma once

#include <unistd.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <rcutils/logging.h>

namespace drill
{

using namespace std::chrono_literals;

/// 他の受講者やテストと衝突しないノード名を作る。
inline std::string unique_name(const std::string & base)
{
  static int counter = 0;
  return base + "_" + std::to_string(getpid()) + "_" + std::to_string(++counter);
}

/// cond() が true になるまで nodes を spin する。
///
/// tick が与えられた場合は spin のたびに呼ばれる（毎周期 publish したい時に使う）。
/// discovery に時間がかかるため、最初の数発は届かない前提で書くこと。
inline bool spin_until(
  const std::vector<rclcpp::Node::SharedPtr> & nodes,
  const std::function<bool()> & cond,
  std::chrono::milliseconds timeout = 5s,
  const std::function<void()> & tick = nullptr)
{
  rclcpp::executors::SingleThreadedExecutor exec;
  for (const auto & node : nodes) {
    exec.add_node(node);
  }

  const auto deadline = std::chrono::steady_clock::now() + timeout;
  bool ok = cond();
  while (!ok && rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
    if (tick) {
      tick();
    }
    // spin_once は 1 回に 1 つしか処理しないため、購読が複数ある構成では
    // 先に登録された側だけが実行され続けてもう一方が飢餓状態になる。
    // spin_all で「今処理できる分を全部」処理する。
    exec.spin_all(20ms);
    ok = cond();
  }

  for (const auto & node : nodes) {
    exec.remove_node(node);
  }
  return ok;
}

/// 指定時間だけ spin する（結果を待たずに一定時間動かしたいとき）。
inline void spin_for(
  const std::vector<rclcpp::Node::SharedPtr> & nodes, std::chrono::milliseconds duration)
{
  spin_until(nodes, []() {return false;}, duration);
}

/// MultiThreadedExecutor で spin しながら cond() を待つ。
///
/// コールバックの中から別のコールバックの完了を待つ構成（サブスクライバの中で
/// サービスを呼ぶなど）は SingleThreadedExecutor だとデッドロックするため、
/// 上級課題のテストではこちらを使う。
inline bool spin_until_multithreaded(
  const std::vector<rclcpp::Node::SharedPtr> & nodes,
  const std::function<bool()> & cond,
  std::chrono::milliseconds timeout = 5s,
  const std::function<void()> & tick = nullptr,
  std::size_t threads = 4)
{
  rclcpp::executors::MultiThreadedExecutor exec(rclcpp::ExecutorOptions(), threads);
  for (const auto & node : nodes) {
    exec.add_node(node);
  }

  std::atomic<bool> spinning_finished{false};
  std::thread spinner(
    [&exec, &spinning_finished]() {
      exec.spin();
      spinning_finished = true;
    });

  const auto deadline = std::chrono::steady_clock::now() + timeout;
  bool ok = cond();
  while (!ok && rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
    if (tick) {
      tick();
    }
    std::this_thread::sleep_for(20ms);
    ok = cond();
  }

  // cancel() は spin() が走り出す前に呼ぶと取りこぼされ、spin() が永久に返らない。
  // （cond() が即 true になるテストで実際にハングした。）
  // spin() が抜けたことを確認できるまで cancel を打ち続ける。
  while (!spinning_finished) {
    exec.cancel();
    std::this_thread::sleep_for(2ms);
  }
  spinner.join();
  for (const auto & node : nodes) {
    exec.remove_node(node);
  }
  return ok;
}

/// RCLCPP_INFO などのログ出力を捕まえる。
///
/// 公式チュートリアルのノードは結果をログに出すだけのものが多いため、
/// ログの内容をテストしたいときに使う。生存期間中だけ有効。
class LogCapture
{
public:
  LogCapture()
  {
    previous_ = rcutils_logging_get_output_handler();
    {
      std::lock_guard<std::mutex> lock(mutex());
      current() = this;
    }
    rcutils_logging_set_output_handler(&LogCapture::handle);
  }

  ~LogCapture()
  {
    rcutils_logging_set_output_handler(previous_);
    std::lock_guard<std::mutex> lock(mutex());
    current() = nullptr;
  }

  LogCapture(const LogCapture &) = delete;
  LogCapture & operator=(const LogCapture &) = delete;

  std::vector<std::string> lines() const
  {
    std::lock_guard<std::mutex> lock(mutex());
    return lines_;
  }

  /// 部分一致で探す。
  bool contains(const std::string & needle) const
  {
    std::lock_guard<std::mutex> lock(mutex());
    for (const auto & line : lines_) {
      if (line.find(needle) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  /// 失敗メッセージに貼るための、捕まえたログ全文。
  std::string dump() const
  {
    std::lock_guard<std::mutex> lock(mutex());
    if (lines_.empty()) {
      return "（ログ出力はありませんでした）";
    }
    std::string out;
    for (const auto & line : lines_) {
      out += "\n      " + line;
    }
    return out;
  }

private:
  static std::mutex & mutex()
  {
    static std::mutex m;
    return m;
  }

  static LogCapture *& current()
  {
    static LogCapture * ptr = nullptr;
    return ptr;
  }

  static void handle(
    const rcutils_log_location_t *, int, const char *, rcutils_time_point_value_t,
    const char * format, va_list * args)
  {
    char buffer[1024];
    va_list copy;
    va_copy(copy, *args);
    vsnprintf(buffer, sizeof(buffer), format, copy);
    va_end(copy);

    std::lock_guard<std::mutex> lock(mutex());
    if (current() != nullptr) {
      current()->lines_.emplace_back(buffer);
    }
  }

  rcutils_logging_output_handler_t previous_;
  mutable std::vector<std::string> lines_;
};

/// 全課題のテストが使う fixture。テストごとに rclcpp を初期化/終了する。
///
/// ノードはテスト本体のローカル変数として作ること
/// （fixture のメンバに持つと shutdown 後に破棄されて警告が出る）。
class DrillTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rclcpp::init(0, nullptr);
  }

  void TearDown() override
  {
    rclcpp::shutdown();
  }
};

}  // namespace drill
