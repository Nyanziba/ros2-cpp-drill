// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>

#include "drill/waveform.hpp"
#include "drill_i18n.hpp"
#include "drill_i18n.hpp"

// --- コンパイル時の検査 -------------------------------------------------
//
// Java では「値がコピーされて派生部分が切り捨てられる」事故が起きません
// （オブジェクトは常に参照だから）。C++ では起きます。
// ヘッダの protected / = delete がその事故を型で止めています。

// 基底への代入はスライシングそのもの。禁止されていること。
static_assert(
  !std::is_copy_assignable<Waveform>::value,
  "Waveform への代入はスライシングになるので禁止されていなければいけません");

// std::unique_ptr<Waveform> をメンバに持つ vector があるので、
// WaveformLibrary の暗黙のコピーはコンパイラが delete します。
// 「丸ごと複製」は duplicate() を通してだけできる、という設計です。
static_assert(
  !std::is_copy_constructible<WaveformLibrary>::value,
  "WaveformLibrary はコピー構築できてはいけません（unique_ptr メンバがあるため）");
static_assert(
  !std::is_copy_assignable<WaveformLibrary>::value,
  "WaveformLibrary はコピー代入できてはいけません");

// ただしムーブはできる。duplicate() が値で返せるのはこれがあるから。
static_assert(
  std::is_move_constructible<WaveformLibrary>::value,
  "WaveformLibrary はムーブ構築できなければいけません");

// PulseTrain は「値としてコピーできる」ことに意味がある型。
// 型が分かっているなら clone() ではなくコピーコンストラクタを使うべきです。
static_assert(
  std::is_copy_constructible<PulseTrain>::value,
  "PulseTrain はコピー構築できなければいけません");

// clone() は unique_ptr<Waveform> を返す。生ポインタを返してはいけません。
static_assert(
  std::is_same<
    decltype(std::declval<const Waveform &>().clone()), std::unique_ptr<Waveform>>::value,
  "clone() は std::unique_ptr<Waveform> を返してください");

namespace
{

constexpr std::size_t kPatternLength = 4;

/// 値が入ったパルス列を 1 つ作る。
PulseTrain make_pulse_train()
{
  PulseTrain pulse{"gripper", kPatternLength};
  for (std::size_t i = 0; i < kPatternLength; ++i) {
    pulse.set_sample(i, static_cast<double>(i) + 1.0);
  }
  return pulse;
}

}  // namespace

TEST(PrototypeTest, CloneReturnsDifferentObject)
{
  std::unique_ptr<Waveform> original = std::make_unique<PulseTrain>(make_pulse_train());

  std::unique_ptr<Waveform> copy = original->clone();

  ASSERT_NE(copy, nullptr) << drill::localized("clone() が nullptr を返しています", "clone() returned nullptr");
  EXPECT_NE(copy.get(), original.get())
    << drill::localized("clone() が同じオブジェクトを指しています。新しい実体を作ってください", "clone() points to the same object. Create a new object");
}

TEST(PrototypeTest, DerivedTypeIsPreservedThroughUniquePtr)
{
  // 呼ぶ側は「Waveform であること」しか知らない。それでも実体の型が複製されるのが Prototype。
  std::unique_ptr<Waveform> original = std::make_unique<PulseTrain>(make_pulse_train());

  std::unique_ptr<Waveform> copy = original->clone();

  ASSERT_NE(copy, nullptr) << drill::localized("clone() が nullptr を返しています", "clone() returned nullptr");
  EXPECT_EQ(copy->name(), "PulseTrain");
  EXPECT_NE(dynamic_cast<PulseTrain *>(copy.get()), nullptr)
    << drill::localized(
         "複製の実体が PulseTrain になっていません。"
         "do_clone() が基底やほかの型を作っていないか確認してください",
         "The copy is not a PulseTrain. "
         "Check that do_clone() does not create the base class or another type");
}

TEST(PrototypeTest, SineSweepCanAlsoBeCloned)
{
  std::unique_ptr<Waveform> original = std::make_unique<SineSweep>(10.0, 200.0, 8);

  std::unique_ptr<Waveform> copy = original->clone();

  ASSERT_NE(copy, nullptr) << drill::localized("clone() が nullptr を返しています", "clone() returned nullptr");
  EXPECT_EQ(copy->name(), "SineSweep");

  SineSweep * swept = dynamic_cast<SineSweep *>(copy.get());
  ASSERT_NE(swept, nullptr) << drill::localized("複製の実体が SineSweep になっていません", "The copy is not a SineSweep");
  EXPECT_DOUBLE_EQ(swept->start_hz(), 10.0);
  EXPECT_DOUBLE_EQ(swept->end_hz(), 200.0);
}

TEST(PrototypeTest, ClonedWaveformIsDeepCopy)
{
  std::unique_ptr<Waveform> original = std::make_unique<PulseTrain>(make_pulse_train());
  std::unique_ptr<Waveform> copy = original->clone();
  ASSERT_NE(copy, nullptr) << drill::localized("clone() が nullptr を返しています", "clone() returned nullptr");

  // 複製したあとで元を書き換える。深いコピーなら複製は影響を受けない。
  dynamic_cast<PulseTrain *>(original.get())->set_sample(0, 999.0);

  EXPECT_DOUBLE_EQ(copy->sample(0), 1.0)
    << drill::localized("元を書き換えたら複製も変わりました。浅いコピーになっています", "Changing the original also changed the copy. It is a shallow copy");
  EXPECT_DOUBLE_EQ(original->sample(0), 999.0);
}

TEST(PrototypeTest, PulseTrainCopyConstructorMakesDeepCopy)
{
  // 型が分かっているならこちらが正解。clone() は要らない。
  PulseTrain original = make_pulse_train();
  PulseTrain copy = original;

  for (std::size_t i = 0; i < kPatternLength; ++i) {
    EXPECT_DOUBLE_EQ(copy.sample(i), static_cast<double>(i) + 1.0)
      << i << drill::localized(" 番目のサンプルが写されていません", " th sample was not copied");
  }
  EXPECT_EQ(copy.label(), "gripper");
}

TEST(PrototypeTest, CloneDoesNotShareBuffer)
{
  PulseTrain original = make_pulse_train();
  std::unique_ptr<Waveform> copy = original.clone();
  ASSERT_NE(copy, nullptr) << drill::localized("clone() が nullptr を返しています", "clone() returned nullptr");

  PulseTrain * copied = dynamic_cast<PulseTrain *>(copy.get());
  ASSERT_NE(copied, nullptr);

  EXPECT_NE(copied->data(), original.data())
    << drill::localized("複製が元と同じ配列を指しています。二重解放の一歩手前です", "The copy points to the same array as the original. This is close to a double free");
}

TEST(PrototypeTest, DuplicatePreservesCountAndTypes)
{
  WaveformLibrary library;
  library.add(std::make_unique<PulseTrain>(make_pulse_train()));
  library.add(std::make_unique<SineSweep>(10.0, 200.0, 8));

  WaveformLibrary copy = library.duplicate();

  ASSERT_EQ(copy.size(), 2u) << drill::localized("duplicate() が全要素を複製していません", "duplicate() did not copy all elements");
  EXPECT_EQ(copy.at(0).name(), "PulseTrain");
  EXPECT_EQ(copy.at(1).name(), "SineSweep");
}

TEST(PrototypeTest, DuplicatedElementsAreNotSharedWithOriginal)
{
  WaveformLibrary library;
  library.add(std::make_unique<PulseTrain>(make_pulse_train()));

  WaveformLibrary copy = library.duplicate();
  ASSERT_EQ(copy.size(), 1u) << drill::localized("duplicate() が全要素を複製していません", "duplicate() did not copy all elements");

  EXPECT_NE(&copy.at(0), &library.at(0))
    << drill::localized("duplicate() が同じオブジェクトを共有しています", "duplicate() shares the same object");
  EXPECT_DOUBLE_EQ(copy.at(0).sample(0), 1.0)
    << drill::localized("複製された要素の状態が写っていません", "The state of the copied element was not carried over");
}
