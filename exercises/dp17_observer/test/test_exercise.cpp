// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drill/sensor_hub.hpp"

namespace
{

/// 受け取った値を共有のログに書くだけの観測者。
class RecordingObserver : public SensorObserver
{
public:
  RecordingObserver(std::string name, std::vector<std::string> * log)
  : name_(std::move(name)),
    log_(log)
  {
  }

  void on_sample(int value_mm) override
  {
    log_->push_back(name_ + ":" + std::to_string(value_mm));
  }

private:
  std::string name_;
  std::vector<std::string> * log_;
};

/// 自分の購読トークンを自分で持つ観測者。
/// C++ で Observer を書くときの正しい形はこれです。
/// このオブジェクトが死ねば、購読も一緒に死にます。
class SelfManagedObserver : public SensorObserver
{
public:
  SelfManagedObserver(SensorHub & hub, std::vector<std::string> * log)
  : log_(log),
    subscription_(hub.subscribe(this))
  {
  }

  void on_sample(int value_mm) override
  {
    log_->push_back("self:" + std::to_string(value_mm));
  }

private:
  std::vector<std::string> * log_;
  // 宣言順に注意。log_ を先に初期化してから購読します。
  Subscription subscription_;
};

/// 通知を受けたときに、指定されたトークンを解除する観測者。
class CancellingObserver : public SensorObserver
{
public:
  CancellingObserver(std::string name, std::vector<std::string> * log, Subscription * victim)
  : name_(std::move(name)),
    log_(log),
    victim_(victim)
  {
  }

  void on_sample(int value_mm) override
  {
    log_->push_back(name_ + ":" + std::to_string(value_mm));
    if (victim_ != nullptr) {
      victim_->reset();
      victim_ = nullptr;
    }
  }

private:
  std::string name_;
  std::vector<std::string> * log_;
  Subscription * victim_;
};

/// 通知を受けたら別の Subject に投げ返す観測者（通知の循環をつくる）。
class RelayObserver : public SensorObserver
{
public:
  RelayObserver(std::string name, std::vector<std::string> * log, SensorHub * peer)
  : name_(std::move(name)),
    log_(log),
    peer_(peer)
  {
  }

  void on_sample(int value_mm) override
  {
    log_->push_back(name_ + ":" + std::to_string(value_mm));
    if (peer_ != nullptr) {
      peer_->publish(value_mm);
    }
  }

private:
  std::string name_;
  std::vector<std::string> * log_;
  SensorHub * peer_;
};

/// 通知を受けたときに新しい観測者を購読させる観測者。
class SubscribingObserver : public SensorObserver
{
public:
  SubscribingObserver(
    std::string name, std::vector<std::string> * log, SensorHub * hub, SensorObserver * newcomer)
  : name_(std::move(name)),
    log_(log),
    hub_(hub),
    newcomer_(newcomer)
  {
  }

  void on_sample(int value_mm) override
  {
    log_->push_back(name_ + ":" + std::to_string(value_mm));
    if (newcomer_ != nullptr) {
      held_.push_back(hub_->subscribe(newcomer_));
      newcomer_ = nullptr;
    }
  }

private:
  std::string name_;
  std::vector<std::string> * log_;
  SensorHub * hub_;
  SensorObserver * newcomer_;
  std::vector<Subscription> held_;
};

}  // namespace

TEST(ObserverTest, 複数の観測者に登録順で通知が届く)
{
  SensorHub hub;
  std::vector<std::string> log;

  RecordingObserver display{"display", &log};
  RecordingObserver logger{"logger", &log};
  RecordingObserver control{"control", &log};

  const Subscription s1 = hub.subscribe(&display);
  const Subscription s2 = hub.subscribe(&logger);
  const Subscription s3 = hub.subscribe(&control);

  EXPECT_EQ(hub.observer_count(), 3u);

  hub.publish(120);

  const std::vector<std::string> expected = {"display:120", "logger:120", "control:120"};
  EXPECT_EQ(log, expected) << "登録順に通知されていません";
}

TEST(ObserverTest, 観測者が先に死んでもSubjectが壊れない)
{
  SensorHub hub;
  std::vector<std::string> log;

  {
    const SelfManagedObserver observer{hub, &log};
    ASSERT_EQ(hub.observer_count(), 1u) << "subscribe() が購読を登録していません";
    hub.publish(10);
  }
  // observer はここで死んだ。トークンも一緒に死んだので購読は自動で切れているはず。

  EXPECT_EQ(hub.observer_count(), 0u)
    << "観測者が死んだのに購読が残っています。宙に浮いたポインタです";

  // ここで落ちたら、死んだオブジェクトを呼んでいます。
  hub.publish(20);

  const std::vector<std::string> expected = {"self:10"};
  EXPECT_EQ(log, expected);
}

TEST(ObserverTest, 明示的に購読解除できる)
{
  SensorHub hub;
  std::vector<std::string> log;

  RecordingObserver display{"display", &log};
  RecordingObserver logger{"logger", &log};

  Subscription display_sub = hub.subscribe(&display);
  const Subscription logger_sub = hub.subscribe(&logger);

  hub.publish(1);
  ASSERT_EQ(log.size(), 2u) << "2 つの観測者に通知が届いていません";

  EXPECT_TRUE(display_sub.active());
  display_sub.reset();
  EXPECT_FALSE(display_sub.active());
  EXPECT_EQ(hub.observer_count(), 1u);

  // 2 回目の reset() でも落ちないこと。
  display_sub.reset();

  log.clear();
  hub.publish(2);

  const std::vector<std::string> expected = {"logger:2"};
  EXPECT_EQ(log, expected) << "解除済みの観測者に通知が来ています";
}

TEST(ObserverTest, 解除済みの観測者には何度publishしても通知が来ない)
{
  SensorHub hub;
  std::vector<std::string> log;

  RecordingObserver logger{"logger", &log};

  {
    RecordingObserver display{"display", &log};
    const Subscription display_sub = hub.subscribe(&display);
    const Subscription logger_sub = hub.subscribe(&logger);
    hub.publish(1);
    ASSERT_EQ(log.size(), 2u);
    log.clear();
  }
  // display も logger_sub もスコープを抜けた。購読はどちらも切れている。

  EXPECT_EQ(hub.observer_count(), 0u);

  hub.publish(2);
  hub.publish(3);
  hub.publish(4);

  EXPECT_TRUE(log.empty()) << "解除済みの観測者に通知が来ています";
}

TEST(ObserverTest, 通知中に他の観測者を解除しても落ちない)
{
  SensorHub hub;
  std::vector<std::string> log;

  RecordingObserver logger{"logger", &log};
  RecordingObserver control{"control", &log};

  Subscription control_sub = hub.subscribe(&control);
  // killer は通知を受けたら control の購読を切る。
  CancellingObserver killer{"killer", &log, &control_sub};

  const Subscription killer_sub = hub.subscribe(&killer);
  const Subscription logger_sub = hub.subscribe(&logger);

  ASSERT_EQ(hub.observer_count(), 3u);

  // control → killer → logger の順に通知される。
  // killer が回している最中のリストから control を消しにいく。
  hub.publish(7);

  const std::vector<std::string> expected = {"control:7", "killer:7", "logger:7"};
  EXPECT_EQ(log, expected) << "通知ループの途中で解除したせいで通知が飛んでいます";
  EXPECT_EQ(hub.observer_count(), 2u) << "通知が終わったら解除済みを片づけてください";

  log.clear();
  hub.publish(8);

  const std::vector<std::string> after = {"killer:8", "logger:8"};
  EXPECT_EQ(log, after);
}

TEST(ObserverTest, 通知中に自分より後ろの観測者を解除するとその回は通知されない)
{
  SensorHub hub;
  std::vector<std::string> log;

  RecordingObserver control{"control", &log};
  Subscription control_sub;

  CancellingObserver killer{"killer", &log, &control_sub};
  const Subscription killer_sub = hub.subscribe(&killer);
  control_sub = hub.subscribe(&control);

  ASSERT_EQ(hub.observer_count(), 2u);

  // killer が先。killer が control を切ってから control の番が来る。
  hub.publish(5);

  const std::vector<std::string> expected = {"killer:5"};
  EXPECT_EQ(log, expected) << "解除したはずの観測者に通知が届いています";
  EXPECT_EQ(hub.observer_count(), 1u);
}

TEST(ObserverTest, 通知が循環しても無限ループしない)
{
  SensorHub left;
  SensorHub right;
  std::vector<std::string> log;

  // left の通知 → right へ publish、right の通知 → left へ publish。
  RelayObserver to_right{"left", &log, &right};
  RelayObserver to_left{"right", &log, &left};

  const Subscription s1 = left.subscribe(&to_right);
  const Subscription s2 = right.subscribe(&to_left);

  // 再入防止が無いとここで固まります。
  left.publish(3);

  const std::vector<std::string> expected = {"left:3", "right:3"};
  EXPECT_EQ(log, expected) << "再入防止が効いていません";

  log.clear();
  right.publish(4);

  const std::vector<std::string> expected2 = {"right:4", "left:4"};
  EXPECT_EQ(log, expected2);
}

TEST(ObserverTest, 通知中に購読した観測者はその回には呼ばれない)
{
  SensorHub hub;
  std::vector<std::string> log;

  RecordingObserver newcomer{"newcomer", &log};
  SubscribingObserver adder{"adder", &log, &hub, &newcomer};

  const Subscription adder_sub = hub.subscribe(&adder);

  hub.publish(1);
  const std::vector<std::string> first = {"adder:1"};
  EXPECT_EQ(log, first) << "通知ループ中に増えた購読をその回で呼んでいます";

  log.clear();
  hub.publish(2);
  const std::vector<std::string> second = {"adder:2", "newcomer:2"};
  EXPECT_EQ(log, second) << "次の回では新しい観測者にも通知してください";
}

TEST(ObserverTest, Subscriptionはムーブできるがコピーできない)
{
  static_assert(
    !std::is_copy_constructible<Subscription>::value,
    "Subscription はコピーできてはいけません。購読は 1 つしかありません");
  static_assert(
    !std::is_copy_assignable<Subscription>::value,
    "Subscription はコピー代入できてはいけません");
  static_assert(
    std::is_nothrow_move_constructible<Subscription>::value,
    "Subscription はムーブできる必要があります");

  SensorHub hub;
  std::vector<std::string> log;

  RecordingObserver display{"display", &log};
  RecordingObserver logger{"logger", &log};

  Subscription moved_from = hub.subscribe(&display);
  Subscription moved_to = std::move(moved_from);

  EXPECT_FALSE(moved_from.active()) << "ムーブ元が購読を持ったままです";
  EXPECT_TRUE(moved_to.active());
  EXPECT_EQ(hub.observer_count(), 1u);

  // ムーブ代入は、今持っている購読を先に解除すること。
  Subscription logger_sub = hub.subscribe(&logger);
  ASSERT_EQ(hub.observer_count(), 2u);

  moved_to = std::move(logger_sub);
  EXPECT_EQ(hub.observer_count(), 1u) << "ムーブ代入で元の購読が解除されていません";

  log.clear();
  hub.publish(9);
  const std::vector<std::string> expected = {"logger:9"};
  EXPECT_EQ(log, expected);
}

TEST(ObserverTest, Subjectが先に死んでもトークンの破棄が安全)
{
  std::vector<std::string> log;
  RecordingObserver display{"display", &log};

  Subscription sub;
  {
    SensorHub hub;
    sub = hub.subscribe(&display);
    EXPECT_TRUE(sub.active());
    hub.publish(1);
    EXPECT_EQ(log.size(), 1u);
  }
  // SensorHub が先に死んだ。トークンだけが残っている状態。

  EXPECT_FALSE(sub.active()) << "Subject が死んだのに購読が生きていることになっています";

  // ここで落ちたら、死んだ Subject を触っています。
  sub.reset();
  // このテストを抜けるときの ~Subscription でも落ちないこと。
}
