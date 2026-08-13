// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

class GainTuner;

/// Memento（結城本 第18章 の Memento クラスに対応）。
///
/// Java 版との違い:
///   - Java は「同じパッケージなら見える」(package private) で
///     narrow interface / wide interface を作り分けます。
///     C++ に package private はないので friend で表現します。
///   - 中身をすべて**値**で持ちます。C++ には値セマンティクスがあるので、
///     値でコピーした時点でスナップショットが成立します。
///     ポインタや参照、std::shared_ptr で持つと共有されてしまい、
///     「あとから元が変わるとスナップショットも変わる」壊れ方をします。
///
/// wide interface（中身へのアクセス）は private。GainTuner だけが見られます。
/// narrow interface（外に見せてよいもの）は label() だけです。
class GainSnapshot
{
public:
  /// Caretaker（履歴を持つ側）が見てよい唯一の情報。
  /// 「どの時点のスナップショットか」を人間が識別するためのラベルです。
  const std::string & label() const { return label_; }

private:
  friend class GainTuner;

  GainSnapshot(double kp, double ki, double kd, std::string label)
  : kp_(kp), ki_(ki), kd_(kd), label_(std::move(label))
  {
  }

  double kp_;
  double ki_;
  double kd_;
  std::string label_;
};

/// マイコン向けの状態表現。POD なので memcpy で保存できます。
///
/// GainSnapshot と違って中身が public です。カプセル化を捨てる代わりに
/// 「trivially copyable であること」を型で保証しています。
/// 固定長リングバッファに memcpy で詰めるにはこの性質が要ります。
struct GainState
{
  double kp;
  double ki;
  double kd;
};

static_assert(
  std::is_trivially_copyable<GainState>::value,
  "GainState は memcpy で保存するので trivially copyable でなければなりません");

/// Originator。PID ゲインを調整する側。
///
/// 【使い方】
///   auto snapshot = tuner.create_snapshot();   // 現在の状態を焼き付ける
///   tuner.set_gains(...);                      // いじる
///   tuner.restore(snapshot);                   // まずかったので戻す
class GainTuner
{
public:
  GainTuner(double kp, double ki, double kd, std::string label)
  : kp_(kp), ki_(ki), kd_(kd), label_(std::move(label))
  {
  }

  void set_gains(double kp, double ki, double kd)
  {
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
  }

  void set_label(std::string label) { label_ = std::move(label); }

  double kp() const { return kp_; }
  double ki() const { return ki_; }
  double kd() const { return kd_; }
  const std::string & label() const { return label_; }

  /// 現在の状態を Memento として返す。値で返すのでこれ自体がスナップショットです。
  GainSnapshot create_snapshot() const;

  /// Memento の状態に戻す。Memento はコピーされるので、戻したあとも再利用できます。
  void restore(const GainSnapshot & snapshot);

  /// Memento の状態に戻す（ムーブ版）。std::string の確保を避けられます。
  ///
  /// 【約束】この関数は snapshot から状態を**奪います**。
  /// 呼んだあと snapshot.label() は空文字列になります。もう一度戻すのには使えません。
  void restore(GainSnapshot && snapshot);

  /// マイコン向け。ラベルを含まない POD だけを取り出す。
  GainState capture_state() const;

  /// マイコン向け。POD から戻す。ラベルは変わりません。
  void restore_state(const GainState & state);

private:
  double kp_;
  double ki_;
  double kd_;
  std::string label_;
};

/// Caretaker のマイコン版。固定長リングバッファ。
///
/// std::vector は使いません。動的確保が走らないこと、
/// 履歴が伸び続けてメモリを食い潰さないことが目的です。
/// 容量を超えたら古いものから黙って捨てます。
class GainHistory
{
public:
  static constexpr std::size_t kCapacity = 4;

  /// 履歴に積む。満杯なら最も古いものを捨てる。
  void push(const GainState & state);

  /// 積まれている数。kCapacity を超えません。
  std::size_t size() const;

  bool empty() const { return size() == 0; }

  /// back_index = 0 が最新、1 が 1 つ前。
  /// back_index >= size() のときは呼ばない前提です。
  GainState recent(std::size_t back_index) const;

private:
  GainState buffer_[kCapacity] = {};
  std::size_t head_ = 0;  // 次に書き込む位置
  std::size_t size_ = 0;
};
