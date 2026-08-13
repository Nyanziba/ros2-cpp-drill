// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <vector>

/// 観測者。距離センサの値を受け取る側のインタフェース。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。
///   - Java の Observer は Observable から「引数なしで呼ばれて自分で取りに行く」
///     プル型もありますが、ここではプッシュ型（値を渡す）にしています。
class SensorObserver
{
public:
  virtual ~SensorObserver() = default;

  /// 新しい距離が届いた。単位はミリメートル。
  virtual void on_sample(int value_mm) = 0;
};

namespace detail
{

/// 購読リストの実体。
///
/// これを SensorHub が直接持たず、shared_ptr で持つのが本章の肝です。
/// Subscription 側は weak_ptr で見ます。こうすると
/// 「Subject（SensorHub）が先に死んでも、あとから Subscription が壊れない」
/// が成り立ちます。生ポインタで持つとここが未定義動作になります。
struct Registry
{
  struct Entry
  {
    std::size_t id = 0;
    /// 解除済みの印は nullptr。通知ループ中に要素を消さないための遅延削除です。
    SensorObserver * observer = nullptr;
  };

  std::vector<Entry> entries;
  std::size_t next_id = 1;
  /// 通知ループの最中か。再入防止と遅延削除の両方に使います。
  bool notifying = false;

  /// id の購読に「解除済み」の印を付ける。
  /// 通知ループ中でなければ、そのまま詰め直す。
  void remove(std::size_t id);

  /// 解除済み（observer == nullptr）の要素を実際に取り除く。
  void compact();
};

}  // namespace detail

/// 購読を表す RAII トークン。
///
/// これが本章の中心です。add_observer が void を返すと、
/// 「誰が、いつ解除するのか」がコードのどこにも書かれません。
/// 購読を **値** として返し、その寿命が購読の寿命になる、という設計にします。
///
/// unique_ptr と同じ性質を持たせます。
///   - コピー禁止（購読は 1 つしかない）
///   - ムーブ可能（メンバに持って移動できる）
///   - デストラクタで自動解除
class Subscription
{
public:
  /// 何も購読していないトークン。
  Subscription() = default;

  /// 破棄で自動的に購読解除する。これがトークン方式の全部です。
  ~Subscription();

  Subscription(const Subscription &) = delete;
  Subscription & operator=(const Subscription &) = delete;

  Subscription(Subscription && other) noexcept;
  Subscription & operator=(Subscription && other) noexcept;

  /// 明示的に購読解除する。デストラクタを待たずに切りたいとき。
  /// 2 回呼んでも安全（2 回目は何もしない）。
  void reset();

  /// まだ購読しているか。
  /// SensorHub が先に死んだ場合も false になります。
  bool active() const;

private:
  friend class SensorHub;

  Subscription(std::weak_ptr<detail::Registry> registry, std::size_t id);

  std::weak_ptr<detail::Registry> registry_;
  std::size_t id_ = 0;
};

/// 距離センサの値を配る Subject。
///
/// 【設計の約束】
///   - subscribe() が返した Subscription を捨てると、その場で購読解除されます。
///     受け取ったら必ずメンバか変数で保持してください。
///   - 通知の順番は登録順です。
///   - 通知中の subscribe() / 解除は安全です。新規購読はその回には通知されません。
///   - publish() の中から publish() を呼んでも無限ループしません（2 回目は無視）。
class SensorHub
{
public:
  SensorHub();

  SensorHub(const SensorHub &) = delete;
  SensorHub & operator=(const SensorHub &) = delete;

  /// observer を購読させ、購読を表すトークンを返す。
  /// observer が nullptr のときは何も購読していないトークンを返す。
  ///
  /// 【重要】observer の所有権は取りません。observer が SensorHub より先に
  /// 死ぬ場合、返したトークンも一緒に死んでいる必要があります。
  Subscription subscribe(SensorObserver * observer);

  /// 全購読者に登録順で通知する。
  void publish(int value_mm);

  /// 現在生きている購読の数。
  std::size_t observer_count() const;

private:
  std::shared_ptr<detail::Registry> registry_;
};
