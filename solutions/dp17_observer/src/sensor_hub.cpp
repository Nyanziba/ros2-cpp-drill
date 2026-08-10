// 解答例
//
// 結城本 第17章 Observer を C++ で書いたもの。
// Java 版との最大の差は「購読者が先に死ぬ」ことです。
// ここではトークン方式（購読を表す RAII オブジェクトを返す）で解いています。

#include "drill/sensor_hub.hpp"

#include <algorithm>
#include <utility>

namespace detail
{

void Registry::remove(std::size_t id)
{
  if (id == 0) {
    return;
  }

  for (Entry & entry : entries) {
    if (entry.id == id) {
      // ここで entries から erase してはいけません。
      // 通知ループが entries を走っている最中かもしれないからです。
      // 印を付けるだけにして、実際の削除はループが終わってから行います。
      entry.observer = nullptr;
      break;
    }
  }

  if (!notifying) {
    compact();
  }
}

void Registry::compact()
{
  entries.erase(
    std::remove_if(
      entries.begin(), entries.end(),
      [](const Entry & entry) { return entry.observer == nullptr; }),
    entries.end());
}

}  // namespace detail

Subscription::Subscription(std::weak_ptr<detail::Registry> registry, std::size_t id)
: registry_(std::move(registry)),
  id_(id)
{
}

Subscription::~Subscription()
{
  // これがトークン方式の全部です。スコープを抜ければ購読が切れます。
  reset();
}

Subscription::Subscription(Subscription && other) noexcept
: registry_(std::move(other.registry_)),
  id_(other.id_)
{
  // ムーブ元を「何も購読していない」状態にします。
  // これを忘れると、ムーブ元が死んだ瞬間に購読が解除されます。
  other.registry_.reset();
  other.id_ = 0;
}

Subscription & Subscription::operator=(Subscription && other) noexcept
{
  if (this != &other) {
    // 先に自分が持っている購読を解除します。
    // unique_ptr の move 代入と同じ形です。
    reset();
    registry_ = std::move(other.registry_);
    id_ = other.id_;
    other.registry_.reset();
    other.id_ = 0;
  }
  return *this;
}

void Subscription::reset()
{
  // lock() が空を返す = SensorHub がもう死んでいる。
  // このとき何もしないのが正解です。生ポインタで持っていたら、
  // ここで死んだ SensorHub を触って未定義動作になります。
  if (const std::shared_ptr<detail::Registry> registry = registry_.lock()) {
    registry->remove(id_);
  }
  registry_.reset();
  id_ = 0;
}

bool Subscription::active() const
{
  return id_ != 0 && !registry_.expired();
}

SensorHub::SensorHub()
: registry_(std::make_shared<detail::Registry>())
{
}

Subscription SensorHub::subscribe(SensorObserver * observer)
{
  if (observer == nullptr) {
    return Subscription{};
  }

  const std::size_t id = registry_->next_id;
  ++registry_->next_id;
  registry_->entries.push_back(detail::Registry::Entry{id, observer});

  // 購読を「値」として返します。呼び出し側が捨てれば、その場で解除されます。
  return Subscription{registry_, id};
}

void SensorHub::publish(int value_mm)
{
  // 再入防止。A の通知先が publish() を呼び返しても、ここで止まります。
  // これが無いと 0.4 節の「A → B → A」で無限ループします。
  if (registry_->notifying) {
    return;
  }

  // 途中で on_sample が例外を投げても notifying を戻すための番人。
  // フラグを直接書き戻すだけだと、例外が飛んだ瞬間に Subject が
  // 「永久に通知しない」状態で固まります。
  struct NotifyingGuard
  {
    explicit NotifyingGuard(detail::Registry & target)
    : registry(target)
    {
      registry.notifying = true;
    }
    ~NotifyingGuard() { registry.notifying = false; }

    NotifyingGuard(const NotifyingGuard &) = delete;
    NotifyingGuard & operator=(const NotifyingGuard &) = delete;

    detail::Registry & registry;
  };

  {
    const NotifyingGuard guard{*registry_};

    // 添字で回します。参照やイテレータを取っておくと、
    // on_sample の中で subscribe() されて vector が再確保された瞬間に
    // それが無効になります。
    // 件数はループ前に固定します。通知中に増えた購読は次回からです。
    const std::size_t count = registry_->entries.size();
    for (std::size_t i = 0; i < count; ++i) {
      SensorObserver * observer = registry_->entries[i].observer;
      if (observer != nullptr) {
        observer->on_sample(value_mm);
      }
    }
  }

  // ループが終わってから、解除済みの分をまとめて片づけます。
  registry_->compact();
}

std::size_t SensorHub::observer_count() const
{
  std::size_t count = 0;
  for (const detail::Registry::Entry & entry : registry_->entries) {
    if (entry.observer != nullptr) {
      ++count;
    }
  }
  return count;
}
