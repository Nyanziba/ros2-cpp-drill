// 解答例。
//
// 結城本 第18章 Memento。C++ には値セマンティクスがあるので、
// 「状態を値で返す」だけでスナップショットが成立します。

#include "drill/gain_tuner.hpp"

#include <cstring>
#include <utility>

GainSnapshot GainTuner::create_snapshot() const
{
  // 4 つとも値で渡す。ここがスナップショットの本体。
  return GainSnapshot{kp_, ki_, kd_, label_};
}

void GainTuner::restore(const GainSnapshot & snapshot)
{
  kp_ = snapshot.kp_;
  ki_ = snapshot.ki_;
  kd_ = snapshot.kd_;
  label_ = snapshot.label_;  // コピー。snapshot は変化しない
}

void GainTuner::restore(GainSnapshot && snapshot)
{
  kp_ = snapshot.kp_;
  ki_ = snapshot.ki_;
  kd_ = snapshot.kd_;
  label_ = std::move(snapshot.label_);  // 奪う。確保が 1 回減る

  // ムーブ後の std::string の中身は規格上「未規定」。
  // ヘッダで「空になる」と約束したので、自分で明示的に空にする。
  snapshot.label_.clear();
}

GainState GainTuner::capture_state() const
{
  return GainState{kp_, ki_, kd_};
}

void GainTuner::restore_state(const GainState & state)
{
  kp_ = state.kp;
  ki_ = state.ki;
  kd_ = state.kd;
  // label_ は触らない。POD 版はゲインだけを扱う。
}

void GainHistory::push(const GainState & state)
{
  // GainState は trivially copyable なので memcpy で書ける。
  // 代入で書いても生成されるコードは同じだが、
  // 「この構造体は memcpy で運べる」という意図を残すためこう書いている。
  std::memcpy(&buffer_[head_], &state, sizeof(GainState));

  head_ = (head_ + 1) % kCapacity;
  if (size_ < kCapacity) {
    ++size_;
  }
}

std::size_t GainHistory::size() const
{
  return size_;
}

GainState GainHistory::recent(std::size_t back_index) const
{
  // head_ は「次に書く位置」なので、最新はその 1 つ手前。
  // 引き算で負にならないよう kCapacity を足してから剰余を取る。
  const std::size_t index =
    (head_ + kCapacity - 1 - back_index) % kCapacity;
  return buffer_[index];
}
