// I AM NOT DONE
//
// 結城本 第17章 Observer を C++ で書きます。
//
// Java 版は addObserver() が void を返して終わりです。C++ でそれをやると、
// 購読者が先に死んだ瞬間に Subject が宙に浮いたポインタを叩きます。
// ここでは「購読を表す RAII トークンを返す」方式で解きます。
//
// 実装するのは 8 か所です。上から順に埋めてください。

#include "drill/sensor_hub.hpp"

#include <algorithm>
#include <utility>

namespace detail
{

void Registry::remove(std::size_t id)
{
  // TODO(1): id に一致する Entry を探して observer に nullptr を入れてください。
  //
  // 【ここで entries.erase() を呼んではいけません】
  //   通知ループ（publish）が entries を走っている最中かもしれません。
  //   要素を消すと、走っているループの添字や参照がずれます。
  //   印を付けるだけにして、実際の削除は notifying が false のときだけ行います。
  //
  //   if (!notifying) { compact(); }
  //
  // id == 0 は「何も購読していない」印なので、その場合は何もしないでください。
  static_cast<void>(id);
}

void Registry::compact()
{
  // TODO(2): observer == nullptr の要素を entries から取り除いてください。
  //          <algorithm> の std::remove_if と erase の組み合わせが素直です。
}

}  // namespace detail

// ここは実装済みです（トークンを作るのは SensorHub::subscribe だけなので private）。
Subscription::Subscription(std::weak_ptr<detail::Registry> registry, std::size_t id)
: registry_(std::move(registry)),
  id_(id)
{
}

Subscription::~Subscription()
{
  // TODO(3): 購読を解除してください。1 行で書けます。
  //          トークン方式の価値はこの 1 行に全部あります。
}

Subscription::Subscription(Subscription && other) noexcept
: registry_(std::move(other.registry_)),
  id_(other.id_)
{
  // TODO(4): このままだと「ムーブ元も同じ購読を持っている」状態です。
  //          ムーブ元が先に死んだ瞬間に購読が切れてしまいます。
  //          ムーブ元を「何も購読していない」状態に戻してください。
}

Subscription & Subscription::operator=(Subscription && other) noexcept
{
  // TODO(5): unique_ptr の move 代入と同じ形にしてください。
  //          1. 自己代入を弾く
  //          2. 今持っている購読を先に解除する
  //          3. other から奪う
  //          4. other を空にする
  registry_ = std::move(other.registry_);
  id_ = other.id_;
  return *this;
}

void Subscription::reset()
{
  // TODO(6): 購読を解除してください。
  //
  // registry_ は weak_ptr です。lock() が空を返したら、
  // SensorHub がもう死んでいます。そのときは何もしないのが正解です。
  // （生ポインタで持っていたら、ここで死んだ Subject を触ります）
  //
  // 解除したら registry_ と id_ を空にしてください。
  // reset() を 2 回呼んでも安全でなければいけません。
}

bool Subscription::active() const
{
  // TODO(7): まだ購読しているなら true。
  //          SensorHub が先に死んでいる場合も false になるようにしてください。
  //          weak_ptr::expired() を使います。
  return false;
}

// ここは実装済みです。
SensorHub::SensorHub()
: registry_(std::make_shared<detail::Registry>())
{
}

Subscription SensorHub::subscribe(SensorObserver * observer)
{
  if (observer == nullptr) {
    return Subscription{};
  }

  // TODO(8a): 新しい id を採番して entries に積み、
  //           Subscription{registry_, id} を返してください。
  return Subscription{};
}

void SensorHub::publish(int value_mm)
{
  // TODO(8b): 登録順に on_sample(value_mm) を呼んでください。
  //
  // 気をつけることが 3 つあります。
  //   1. すでに notifying なら、何もせず戻る（再入防止）。
  //      これが無いと A → B → A の通知で無限ループします。
  //   2. ループ中は notifying を true にする。
  //      戻し忘れを防ぐため、デストラクタで false に戻す小さな RAII を
  //      その場で書くのが安全です（on_sample が例外を投げても戻ります）。
  //   3. 参照やイテレータを取り置きしない。添字で回す。
  //      通知中に subscribe されると vector が再確保されます。
  //      件数はループ前に固定します（通知中に増えた分は次回から）。
  //
  // ループが終わったら compact() で解除済みを片づけてください。
  static_cast<void>(value_mm);
}

std::size_t SensorHub::observer_count() const
{
  // TODO(8c): observer が nullptr でない Entry の数を返してください。
  return 0;
}
