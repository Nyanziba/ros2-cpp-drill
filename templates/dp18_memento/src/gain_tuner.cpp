// I AM NOT DONE
//
// 結城本 第18章 Memento を C++ で書きます。
// Java 版と違い、C++ には値セマンティクスがあります。
// 「状態を値で返す」だけでスナップショットが成立します。
// コピーを避けようとしてポインタや参照を持たせた瞬間に壊れます。

#include "drill/gain_tuner.hpp"

#include <cstring>

GainSnapshot GainTuner::create_snapshot() const
{
  // TODO: 現在の kp_ / ki_ / kd_ / label_ を持つ GainSnapshot を作って返してください。
  //
  // GainSnapshot のコンストラクタは private ですが、GainTuner は friend なので呼べます。
  // 4 つとも「値で」渡してください。参照やポインタを持たせるとスナップショットになりません。
  return GainSnapshot{0.0, 0.0, 0.0, "TODO"};
}

void GainTuner::restore(const GainSnapshot & snapshot)
{
  // TODO: snapshot の中身を自分に書き戻してください。
  //
  // snapshot は const 参照なので、label_ は「コピー」で受け取ります。
  // snapshot 側は変化しません（あとでもう一度戻せます）。

  // 未実装のあいだ -Wunused-private-field を出さないための行です。実装したら消してください。
  (void)(snapshot.kp_ + snapshot.ki_ + snapshot.kd_);
}

void GainTuner::restore(GainSnapshot && snapshot)
{
  // TODO: ムーブ版。std::string の確保を 1 回節約します。
  //
  // label_ は std::move(snapshot.label_) で奪ってください。
  // 奪ったあと、ヘッダの約束どおり snapshot.label_ を clear() して
  // 「空になっている」ことを保証してください。
  // （ムーブ後の std::string の中身は規格上「未規定」です。
  //   使う側に約束するなら自分で明示的に空にします。）
  (void)snapshot;
}

GainState GainTuner::capture_state() const
{
  // TODO: kp_ / ki_ / kd_ だけを詰めた GainState を返してください。
  return GainState{0.0, 0.0, 0.0};
}

void GainTuner::restore_state(const GainState & state)
{
  // TODO: state の 3 つの値を自分に書き戻してください。label_ は触りません。
  (void)state;
}

void GainHistory::push(const GainState & state)
{
  // TODO: リングバッファに 1 件積んでください。
  //
  //   1. buffer_[head_] に state を書く
  //      （GainState は trivially copyable なので、代入でも std::memcpy でも同じです。
  //        マイコンで「状態が構造体の塊」なら memcpy 版が素直です）
  //   2. head_ を 1 つ進める。kCapacity に達したら 0 に戻す
  //   3. size_ を 1 増やす。ただし kCapacity を超えないようにする
  buffer_[head_] = state;
}

std::size_t GainHistory::size() const
{
  // TODO: 積まれている件数を返してください。
  return size_;
}

GainState GainHistory::recent(std::size_t back_index) const
{
  // TODO: back_index = 0 が最新、1 が 1 つ前になるように返してください。
  //
  // head_ は「次に書く位置」なので、最新は head_ の 1 つ手前です。
  // 添字が負にならないよう、kCapacity を足してから % を取ってください。
  return buffer_[back_index];
}
