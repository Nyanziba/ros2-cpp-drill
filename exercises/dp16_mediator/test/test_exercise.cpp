// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "drill/control_panel.hpp"

namespace
{

/// テスト用の最小 Mediator。誰から報告が来たかだけを覚える。
class RecordingMediator : public PanelMediator
{
public:
  void widget_changed(PanelWidget * widget) override
  {
    reported_names.push_back(widget->name());
  }

  std::vector<std::string> reported_names;
};

}  // namespace

TEST(MediatorTest, 生成直後に4つの部品がMediatorと結線されている)
{
  ControlPanel panel;

  EXPECT_TRUE(panel.emergency_stop().has_mediator())
    << "コンストラクタで set_mediator(this) を呼んでいますか";
  EXPECT_TRUE(panel.auto_mode().has_mediator());
  EXPECT_TRUE(panel.manual_forward().has_mediator());
  EXPECT_TRUE(panel.manual_stop().has_mediator());

  EXPECT_TRUE(panel.emergency_stop().is_enabled());
  EXPECT_TRUE(panel.auto_mode().is_enabled());
  EXPECT_TRUE(panel.manual_forward().is_enabled());
  EXPECT_TRUE(panel.manual_stop().is_enabled());
}

TEST(MediatorTest, 自動モードをオンにすると手動ボタンがMediator経由で無効になる)
{
  ControlPanel panel;

  panel.auto_mode().set_checked(true);

  EXPECT_TRUE(panel.auto_mode().is_checked());
  EXPECT_FALSE(panel.manual_forward().is_enabled());
  EXPECT_FALSE(panel.manual_stop().is_enabled());
  EXPECT_TRUE(panel.auto_mode().is_enabled());
}

TEST(MediatorTest, 自動モードをオフに戻すと手動ボタンが有効に戻る)
{
  ControlPanel panel;

  panel.auto_mode().set_checked(true);
  ASSERT_FALSE(panel.manual_forward().is_enabled());

  panel.auto_mode().set_checked(false);

  EXPECT_FALSE(panel.auto_mode().is_checked());
  EXPECT_TRUE(panel.manual_forward().is_enabled());
  EXPECT_TRUE(panel.manual_stop().is_enabled());
}

TEST(MediatorTest, 非常停止で非常停止トグル以外がすべて無効になる)
{
  ControlPanel panel;

  panel.emergency_stop().set_checked(true);

  EXPECT_TRUE(panel.emergency_stop().is_enabled()) << "解除できなくなります";
  EXPECT_FALSE(panel.auto_mode().is_enabled());
  EXPECT_FALSE(panel.manual_forward().is_enabled());
  EXPECT_FALSE(panel.manual_stop().is_enabled());

  // 無効なトグルは操作できない。
  panel.auto_mode().set_checked(true);
  EXPECT_FALSE(panel.auto_mode().is_checked());
}

TEST(MediatorTest, 非常停止を解除すると自動モードの状態に応じて戻る)
{
  ControlPanel panel;

  panel.auto_mode().set_checked(true);
  panel.emergency_stop().set_checked(true);
  panel.emergency_stop().set_checked(false);

  // 自動モードはオンのままなので、手動ボタンは無効のまま。
  EXPECT_TRUE(panel.auto_mode().is_checked());
  EXPECT_TRUE(panel.auto_mode().is_enabled());
  EXPECT_FALSE(panel.manual_forward().is_enabled());

  panel.auto_mode().set_checked(false);
  EXPECT_TRUE(panel.manual_forward().is_enabled());
}

TEST(MediatorTest, 無効なボタンは押しても押下回数が増えない)
{
  ControlPanel panel;

  EXPECT_TRUE(panel.manual_forward().press());
  EXPECT_EQ(panel.manual_forward().press_count(), 1);

  panel.auto_mode().set_checked(true);

  EXPECT_FALSE(panel.manual_forward().press());
  EXPECT_EQ(panel.manual_forward().press_count(), 1);
}

TEST(MediatorTest, 変化の報告はMediatorに1回だけ届く)
{
  ControlPanel panel;
  panel.clear_change_log();

  panel.auto_mode().set_checked(true);

  const std::vector<std::string> expected = {"auto_mode"};
  EXPECT_EQ(panel.change_log(), expected)
    << "set_enabled() から notify_changed() を呼んでいませんか（無限再帰の一歩手前です）";
}

TEST(MediatorTest, 同じ値をもう一度入れても報告されない)
{
  ControlPanel panel;

  panel.auto_mode().set_checked(true);
  ASSERT_TRUE(panel.auto_mode().is_checked());

  panel.clear_change_log();
  panel.auto_mode().set_checked(true);

  EXPECT_TRUE(panel.change_log().empty());
}

TEST(MediatorTest, Mediatorを外すとColleague間に影響が伝わらない)
{
  ControlPanel panel;
  panel.clear_change_log();

  // 自動モードだけ結線を切る。Colleague どうしが直接つながっていれば影響は残るはず。
  panel.auto_mode().set_mediator(nullptr);
  panel.auto_mode().set_checked(true);

  EXPECT_TRUE(panel.auto_mode().is_checked());
  EXPECT_TRUE(panel.manual_forward().is_enabled())
    << "Colleague どうしが直接やり取りしています。必ず Mediator を経由させてください";
  EXPECT_TRUE(panel.change_log().empty());
}

TEST(MediatorTest, Mediator未設定のColleagueは報告しても落ちない)
{
  ToggleWidget orphan{"orphan"};

  EXPECT_FALSE(orphan.has_mediator());
  orphan.set_checked(true);
  EXPECT_TRUE(orphan.is_checked());
}

TEST(MediatorTest, ColleagueはMediatorを所有しない)
{
  auto mediator = std::make_shared<RecordingMediator>();
  ToggleWidget toggle{"solo"};

  toggle.set_mediator(mediator.get());

  // shared_ptr を持ち返していたら use_count は 2 になり、循環参照の一歩手前です。
  EXPECT_EQ(mediator.use_count(), 1)
    << "Colleague が Mediator の所有権を持っています。生ポインタで指すだけにしてください";

  toggle.set_checked(true);
  const std::vector<std::string> expected = {"solo"};
  EXPECT_EQ(mediator->reported_names, expected);
}

TEST(MediatorTest, Mediatorの破棄でColleagueもすべて破棄される)
{
  LifetimeLog::instance().clear();

  {
    ControlPanel panel;
    ASSERT_TRUE(panel.auto_mode().has_mediator());
    panel.auto_mode().set_checked(true);
    EXPECT_TRUE(LifetimeLog::instance().entries().empty());
  }

  // ControlPanel 本体 → メンバ（宣言と逆順）の順に解放される。
  const std::vector<std::string> expected = {
    "ControlPanel", "manual_stop", "manual_forward", "auto_mode", "emergency_stop"};
  EXPECT_EQ(LifetimeLog::instance().entries(), expected)
    << "解放されていない Colleague があります。循環参照を疑ってください";

  LifetimeLog::instance().clear();
}
