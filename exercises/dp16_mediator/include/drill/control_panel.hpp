// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>
#include <string>
#include <vector>

/// デストラクタが呼ばれたことを記録するための、テスト用の観測点。
///
/// 「循環参照でオブジェクトが解放されない」ことは、目で見ないと信じられません。
/// 各オブジェクトのデストラクタがここに自分の名前を残すので、
/// テストとサンプルコードの両方から「本当に死んだか」を確認できます。
class LifetimeLog
{
public:
  static LifetimeLog & instance();

  void record(std::string name);
  const std::vector<std::string> & entries() const;
  void clear();

  LifetimeLog(const LifetimeLog &) = delete;
  LifetimeLog & operator=(const LifetimeLog &) = delete;

private:
  LifetimeLog() = default;

  std::vector<std::string> entries_;
};

class PanelMediator;

/// Colleague（結城本の Colleague インタフェース + 共通実装）。
///
/// Java 版との違い:
///   - Mediator への参照を **生ポインタ** で持ちます。
///     Mediator が Colleague を std::shared_ptr で持ち、Colleague が Mediator を
///     std::shared_ptr で持ち返すと、参照カウントが 0 にならず両方とも解放されません。
///     所有の向きは Mediator → Colleague の 1 方向だけです。
///   - Java の interface と違い、共通の状態（名前・有効フラグ）を基底に置いています。
///     Colleague ごとに同じコードを書かないためです。
///   - 純粋仮想関数を持たなくても、派生させる基底なので仮想デストラクタは必須です。
///
/// 【寿命の約束】
/// set_mediator() で渡した PanelMediator は、この Colleague より長生きすること。
/// ControlPanel は自分が所有する Colleague にだけ自分を渡すので、この約束は自動的に守られます。
class PanelWidget
{
public:
  explicit PanelWidget(std::string name);
  virtual ~PanelWidget();

  // Colleague は Mediator から指される側です。コピーされると
  // 「どちらが Mediator に登録されている方か」が分からなくなるので禁止します。
  PanelWidget(const PanelWidget &) = delete;
  PanelWidget & operator=(const PanelWidget &) = delete;

  /// 部品の名前（"auto_mode" など）。Mediator が誰から来た通知かを判断するのに使います。
  const std::string & name() const;

  /// 2 段階初期化。コンストラクタでは繋げません（記事 16.4 参照）。
  /// 所有はしません。生ポインタなのは「所有しない」ことを型で言うためです。
  void set_mediator(PanelMediator * mediator);

  /// Mediator が設定済みか。未設定の Colleague は「誰にも報告できない」状態です。
  bool has_mediator() const;

  /// 操作できる状態か。
  bool is_enabled() const;

  /// 有効・無効を切り替える。**Mediator から呼ばれる側**の入口です。
  /// ここから notify_changed() を呼んではいけません（無限再帰になります）。
  void set_enabled(bool enabled);

protected:
  /// 自分が変化したことを Mediator に報告する。**Colleague から呼ぶ側**の入口です。
  /// Mediator 未設定なら何も起きません。
  void notify_changed();

private:
  std::string name_;
  PanelMediator * mediator_ = nullptr;
  bool is_enabled_ = true;
};

/// チェックボックス相当の Colleague（"自動モード"、"非常停止"）。
class ToggleWidget : public PanelWidget
{
public:
  explicit ToggleWidget(std::string name);

  bool is_checked() const;

  /// チェック状態を変える。無効な部品は操作できません（何も起きない）。
  /// 状態が実際に変わったときだけ Mediator に報告します。
  void set_checked(bool checked);

private:
  bool is_checked_ = false;
};

/// 押しボタン相当の Colleague（"手動前進"、"手動停止"）。
class ButtonWidget : public PanelWidget
{
public:
  explicit ButtonWidget(std::string name);

  /// 押された回数。
  int press_count() const;

  /// 押す。無効なら押せずに false を返し、押下回数も増えません。
  bool press();

private:
  int press_count_ = 0;
};

/// Mediator（結城本の Mediator インタフェース）。
///
/// Colleague はこの型しか知りません。ControlPanel という具体型を知らないので、
/// Colleague どうしが直接つながることはありません。
class PanelMediator
{
public:
  virtual ~PanelMediator() = default;

  /// widget が変化したことの報告。widget は「誰が」を伝えるだけで、所有権は動きません。
  virtual void widget_changed(PanelWidget * widget) = 0;
};

/// ConcreteMediator。ロボットの操作パネル。
///
/// 調停ルール:
///   - 非常停止がオンなら、非常停止トグル以外はすべて無効
///   - 非常停止トグル自体は常に有効（無効にすると解除できなくなる）
///   - 自動モードがオンなら、手動ボタンは無効
///   - どちらもオフなら、すべて有効
///
/// 所有の向き: ControlPanel が Colleague を std::unique_ptr で所有します。
/// Colleague は ControlPanel を生ポインタで指すだけです。逆向きの所有はありません。
class ControlPanel : public PanelMediator
{
public:
  ControlPanel();
  ~ControlPanel() override;

  ControlPanel(const ControlPanel &) = delete;
  ControlPanel & operator=(const ControlPanel &) = delete;

  // Colleague は ControlPanel の所有物です。参照だけを外に出します
  // （unique_ptr を外に渡すと所有が動いてしまいます）。
  ToggleWidget & emergency_stop();
  ToggleWidget & auto_mode();
  ButtonWidget & manual_forward();
  ButtonWidget & manual_stop();

  void widget_changed(PanelWidget * widget) override;

  /// どの Colleague から報告が来たかの記録（テストと学習用）。
  const std::vector<std::string> & change_log() const;
  void clear_change_log();

private:
  /// 現在のトグル状態から、各部品の有効・無効を全部決め直す。
  void update_enabled_states();

  std::unique_ptr<ToggleWidget> emergency_stop_;
  std::unique_ptr<ToggleWidget> auto_mode_;
  std::unique_ptr<ButtonWidget> manual_forward_;
  std::unique_ptr<ButtonWidget> manual_stop_;
  std::vector<std::string> change_log_;
};
