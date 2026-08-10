// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// 受け手（Receiver）。実装済みです。読むだけでよい。
// ---------------------------------------------------------------------------

/// テレオペ対象のロボットアーム。結城本の Drawable / Canvas に相当します。
///
/// 実行された操作を log_ に文字列で残します。テストが
/// 「積んだ順に実行されたか」「undo が逆順に効いたか」を
/// **呼び出しの並び**で確かめるためです。角度だけ見ると足し算は順序に依らないので、
/// 順序のバグが素通りしてしまいます。
class RobotArm
{
public:
  void rotate(double delta_deg)
  {
    angle_deg_ += delta_deg;
    log_.push_back("rotate " + std::to_string(static_cast<int>(delta_deg)));
  }

  void set_gripper(bool closed)
  {
    gripper_closed_ = closed;
    log_.push_back(closed ? "grip" : "release");
  }

  double angle_deg() const { return angle_deg_; }
  bool gripper_closed() const { return gripper_closed_; }

  const std::vector<std::string> & log() const { return log_; }
  void clear_log() { log_.clear(); }

private:
  double angle_deg_ = 0.0;
  bool gripper_closed_ = false;
  std::vector<std::string> log_;
};

// ---------------------------------------------------------------------------
// 1. クラス版の Command
// ---------------------------------------------------------------------------

/// 命令を表す抽象クラス。結城本の Command インタフェースに対応します。
///
/// 【Java 版との違い】
///   - 仮想デストラクタが要ります。unique_ptr<Command> で破棄するためです。
///   - Java の Command は execute() だけですが、undo を持たせるので 2 つになります。
///     **操作が 1 つで済むなら、C++ では std::function<void()> で足ります。**
///     クラスにする理由は「実行と取り消しという 2 つの操作を 1 つの型に束ねたい」ことです。
class Command
{
public:
  virtual ~Command() = default;

  /// 操作を実行する。
  virtual void execute() = 0;

  /// 直前の execute() を取り消す。execute() と対で呼ばれることを前提にします。
  virtual void undo() = 0;

  /// ログ用の名前。テストがこれを見ます。
  virtual std::string name() const = 0;
};

/// 相対回転。**逆操作が自明**なコマンドです（+30 の逆は -30）。
/// 実行前の状態を保存する必要がありません。ここが Memento（第18章）との分かれ目です。
///
/// arm_ は生ポインタです。**このコマンドはアームを所有しません。**
/// アームより長生きさせるとぶら下がりポインタになります（第17章 Observer と同じ問題）。
class RotateCommand : public Command
{
public:
  RotateCommand(RobotArm & arm, double delta_deg)
  : arm_(&arm),
    delta_deg_(delta_deg)
  {
  }

  void execute() override;
  void undo() override;
  std::string name() const override;

private:
  RobotArm * arm_;
  double delta_deg_;
};

/// グリッパの開閉。**逆操作が自明でない**コマンドです。
/// 「閉じる」の逆は「開く」とは限りません。もともと閉じていたなら閉じたままが正解です。
///
/// そこで execute() のときに直前の状態を previous_ に控えます。
/// これは Command の中に小さな Memento を持っている形です。
class GripperCommand : public Command
{
public:
  GripperCommand(RobotArm & arm, bool closed)
  : arm_(&arm),
    closed_(closed)
  {
  }

  void execute() override;
  void undo() override;
  std::string name() const override;

  /// execute() が控えた「実行前の状態」。テストが覗きます。
  bool previous() const { return previous_; }

private:
  RobotArm * arm_;
  bool closed_;
  bool previous_ = false;
};

/// マクロコマンド。複数のコマンドを 1 つのコマンドとして扱います。
/// Command を実装しつつ Command を持つので、これは Composite（第11章）そのものです。
///
/// 子は unique_ptr で**所有します**。RotateCommand が RobotArm を所有しないのと対照的です。
class MacroCommand : public Command
{
public:
  /// 末尾に追加する。追加した順に execute されます。
  void add(std::unique_ptr<Command> command);

  std::size_t size() const { return commands_.size(); }

  void execute() override;
  void undo() override;
  std::string name() const override;

private:
  std::vector<std::unique_ptr<Command>> commands_;
};

/// 実行履歴（Invoker）。undo と redo を持ちます。
///
/// done_   … 実行済みで、まだ取り消していないコマンド。末尾が最後に実行したもの
/// undone_ … undo で取り消したコマンド。末尾が最後に取り消したもの
///
/// 新しいコマンドを run() したら undone_ は捨てます。
/// 分岐した歴史に redo できてしまうと、状態が復元できないためです。
class CommandHistory
{
public:
  /// command を実行して done_ に積む。undone_ は捨てる。
  /// command が nullptr なら何もしない。
  void run(std::unique_ptr<Command> command);

  /// done_ の末尾を undo() して undone_ へ移す。何も無ければ false。
  bool undo();

  /// undone_ の末尾を execute() して done_ へ戻す。何も無ければ false。
  bool redo();

  std::size_t undo_depth() const { return done_.size(); }
  std::size_t redo_depth() const { return undone_.size(); }

private:
  std::vector<std::unique_ptr<Command>> done_;
  std::vector<std::unique_ptr<Command>> undone_;
};

// ---------------------------------------------------------------------------
// 2. std::function 版の Command（undo が要らない場合）
// ---------------------------------------------------------------------------

/// 「実行するだけ」でよいなら、コマンドはクラスにする必要がありません。
/// std::function<void()> がそのまま Command です。
///
/// ただし undo は書けません。std::function は操作を 1 つしか包めないからです。
/// undo が要ると分かった時点で、上の Command クラスに戻ってください。
///
/// 【落とし穴】
///   - std::function はキャプチャが大きいとヒープを確保します。
///   - 包む対象はコピー構築可能でなければなりません（unique_ptr のキャプチャは入りません。
///     C++23 の std::move_only_function を待つか、shared_ptr にします）。
///   - **ラムダの参照キャプチャは寿命を延ばしません。**
///     [&arm] で包んだものをキューに積み、arm が先に死ぬと未定義動作です。
class ActionQueue
{
public:
  /// 末尾に積む。
  void push(std::function<void()> action);

  std::size_t size() const { return actions_.size(); }
  bool empty() const { return actions_.empty(); }

  /// 積んだ順に全部実行し、キューを空にする。
  void run_all();

private:
  std::vector<std::function<void()>> actions_;
};

// ---------------------------------------------------------------------------
// 3. マイコン版の Command（動的確保なし・仮想関数なし）
// ---------------------------------------------------------------------------

/// コマンドの種別。virtual の代わりに enum で分岐します。
enum class MotorCommandKind : std::uint8_t
{
  kNone = 0,
  kRotate,
  kGrip,
  kRelease,
};

/// POD のコマンド。**割り込みハンドラから積める形**です。
/// unique_ptr も std::function も std::string も入っていません。4 バイトの値です。
struct MotorCommand
{
  MotorCommandKind kind = MotorCommandKind::kNone;
  std::int16_t argument = 0;
};

/// 固定長のコマンドリングバッファ。ISR で push、メインループで pop する定番の形です。
///
/// 満杯のときの挙動を**設計判断として決めます**。ここでは
/// 「最も古いコマンドを落として新しいものを入れる」を選びます。
/// テレオペの指令は最新が正義なので、古い指令が残る方が危険だからです。
/// （逆に「1 つも落とさない」が要件なら、push は false を返して呼び出し側に再送させます）
class MotorCommandRing
{
public:
  static constexpr std::size_t kCapacity = 4;

  /// 末尾に積む。満杯だったら最も古いものを 1 つ捨てて入れ、false を返す。
  /// 捨てずに入ったときは true。
  bool push(const MotorCommand & command);

  /// 最も古いものを取り出して out に書き、true を返す。空なら out を触らず false。
  bool pop(MotorCommand & out);

  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

private:
  MotorCommand buffer_[kCapacity] = {};
  std::size_t head_ = 0;  ///< 次に書く位置
  std::size_t size_ = 0;
};

/// POD のコマンドをアームに適用する。virtual の代わりの switch です。
/// kRotate の argument は度。kGrip / kRelease は argument を見ません。
void apply(RobotArm & arm, const MotorCommand & command);
