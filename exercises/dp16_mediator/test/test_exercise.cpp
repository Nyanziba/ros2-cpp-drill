// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill_i18n.hpp"

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

TEST(MediatorTest, FourPartsAreWiredToMediatorAfterConstruction)
{
  ControlPanel panel;

  EXPECT_TRUE(panel.emergency_stop().has_mediator())
    << drill::localized("コンストラクタで set_mediator(this) を呼んでいますか", "Does the constructor call set_mediator(this)?");
  EXPECT_TRUE(panel.auto_mode().has_mediator());
  EXPECT_TRUE(panel.manual_forward().has_mediator());
  EXPECT_TRUE(panel.manual_stop().has_mediator());

  EXPECT_TRUE(panel.emergency_stop().is_enabled());
  EXPECT_TRUE(panel.auto_mode().is_enabled());
  EXPECT_TRUE(panel.manual_forward().is_enabled());
  EXPECT_TRUE(panel.manual_stop().is_enabled());
}

TEST(MediatorTest, AutoModeOnDisablesManualButtonsViaMediator)
{
  ControlPanel panel;

  panel.auto_mode().set_checked(true);

  EXPECT_TRUE(panel.auto_mode().is_checked());
  EXPECT_FALSE(panel.manual_forward().is_enabled());
  EXPECT_FALSE(panel.manual_stop().is_enabled());
  EXPECT_TRUE(panel.auto_mode().is_enabled());
}

TEST(MediatorTest, AutoModeOffEnablesManualButtonsAgain)
{
  ControlPanel panel;

  panel.auto_mode().set_checked(true);
  ASSERT_FALSE(panel.manual_forward().is_enabled());

  panel.auto_mode().set_checked(false);

  EXPECT_FALSE(panel.auto_mode().is_checked());
  EXPECT_TRUE(panel.manual_forward().is_enabled());
  EXPECT_TRUE(panel.manual_stop().is_enabled());
}

TEST(MediatorTest, EmergencyStopDisablesEverythingExceptItsToggle)
{
  ControlPanel panel;

  panel.emergency_stop().set_checked(true);

  EXPECT_TRUE(panel.emergency_stop().is_enabled()) << drill::localized("解除できなくなります", "The emergency stop could not be released");
  EXPECT_FALSE(panel.auto_mode().is_enabled());
  EXPECT_FALSE(panel.manual_forward().is_enabled());
  EXPECT_FALSE(panel.manual_stop().is_enabled());

  // 無効なトグルは操作できない。
  panel.auto_mode().set_checked(true);
  EXPECT_FALSE(panel.auto_mode().is_checked());
}

TEST(MediatorTest, ReleasingEmergencyStopRestoresByAutoModeState)
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

TEST(MediatorTest, DisabledButtonDoesNotCountPresses)
{
  ControlPanel panel;

  EXPECT_TRUE(panel.manual_forward().press());
  EXPECT_EQ(panel.manual_forward().press_count(), 1);

  panel.auto_mode().set_checked(true);

  EXPECT_FALSE(panel.manual_forward().press());
  EXPECT_EQ(panel.manual_forward().press_count(), 1);
}

TEST(MediatorTest, ChangeIsReportedToMediatorExactlyOnce)
{
  ControlPanel panel;
  panel.clear_change_log();

  panel.auto_mode().set_checked(true);

  const std::vector<std::string> expected = {"auto_mode"};
  EXPECT_EQ(panel.change_log(), expected)
    << drill::localized("set_enabled() から notify_changed() を呼んでいませんか（無限再帰の一歩手前です）", "Does set_enabled() call notify_changed()? (This is close to infinite recursion)");
}

TEST(MediatorTest, SettingSameValueAgainIsNotReported)
{
  ControlPanel panel;

  panel.auto_mode().set_checked(true);
  ASSERT_TRUE(panel.auto_mode().is_checked());

  panel.clear_change_log();
  panel.auto_mode().set_checked(true);

  EXPECT_TRUE(panel.change_log().empty());
}

TEST(MediatorTest, WithoutMediatorColleaguesDoNotAffectEachOther)
{
  ControlPanel panel;
  panel.clear_change_log();

  // 自動モードだけ結線を切る。Colleague どうしが直接つながっていれば影響は残るはず。
  panel.auto_mode().set_mediator(nullptr);
  panel.auto_mode().set_checked(true);

  EXPECT_TRUE(panel.auto_mode().is_checked());
  EXPECT_TRUE(panel.manual_forward().is_enabled())
    << drill::localized("Colleague どうしが直接やり取りしています。必ず Mediator を経由させてください", "Colleagues talk to each other directly. Always go through the Mediator");
  EXPECT_TRUE(panel.change_log().empty());
}

TEST(MediatorTest, ColleagueWithoutMediatorDoesNotCrashOnReport)
{
  ToggleWidget orphan{"orphan"};

  EXPECT_FALSE(orphan.has_mediator());
  orphan.set_checked(true);
  EXPECT_TRUE(orphan.is_checked());
}

TEST(MediatorTest, ColleagueDoesNotOwnMediator)
{
  auto mediator = std::make_shared<RecordingMediator>();
  ToggleWidget toggle{"solo"};

  toggle.set_mediator(mediator.get());

  // shared_ptr を持ち返していたら use_count は 2 になり、循環参照の一歩手前です。
  EXPECT_EQ(mediator.use_count(), 1)
    << drill::localized("Colleague が Mediator の所有権を持っています。生ポインタで指すだけにしてください", "A Colleague owns the Mediator. Only point to it with a raw pointer");

  toggle.set_checked(true);
  const std::vector<std::string> expected = {"solo"};
  EXPECT_EQ(mediator->reported_names, expected);
}

TEST(MediatorTest, DestroyingMediatorDestroysAllColleagues)
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
    << drill::localized("解放されていない Colleague があります。循環参照を疑ってください", "Some Colleagues were not freed. Suspect a reference cycle");

  LifetimeLog::instance().clear();
}
