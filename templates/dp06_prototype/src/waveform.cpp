// I AM NOT DONE
//
// 結城本 第6章 Prototype を C++ で書きます。
//
// 実装するのは 5 か所です。
//   1. Waveform::clone()            do_clone() の戻り値を unique_ptr に包む
//   2. PulseTrain のコピーコンストラクタ   深いコピー
//   3. PulseTrain::do_clone()       共変戻り値型
//   4. SineSweep::do_clone()        Rule of Zero
//   5. WaveformLibrary::duplicate() 実体の型を知らないまま全部複製する
//
// ヘッダ include/drill/waveform.hpp は編集しません。
// なぜ do_clone() が生ポインタを返しているのかは、ヘッダのコメントを読んでください。

#include "drill/waveform.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

std::unique_ptr<Waveform> Waveform::clone() const
{
  // TODO: do_clone() を呼び、その戻り値を std::unique_ptr<Waveform> に包んで返してください。
  //
  // ヒント: do_clone() は生ポインタ（Waveform *）を返します。
  //         std::unique_ptr<Waveform>(...) のコンストラクタに渡すだけです。
  //         std::make_unique は使えません。実体の型が分からないからです。
  //
  // 今は nullptr を返しているので、clone() を使うテストは全部落ちます。
  return nullptr;
}

PulseTrain::PulseTrain(std::string label, std::size_t length)
: label_(std::move(label)), pattern_(new double[length]()), length_(length)
{
  // ここは実装済みです。
}

PulseTrain::PulseTrain(const PulseTrain & other)
: Waveform(other),                          // 基底の protected なコピーコンストラクタ
  label_(other.label_),
  pattern_(new double[other.length_]()),    // 新しい配列は確保済み
  length_(other.length_)
{
  // TODO: other.pattern_ の中身を pattern_ に 1 つずつ写してください。
  //
  // ヒント: std::copy(other.pattern_.get(), other.pattern_.get() + other.length_,
  //                   pattern_.get());
  //
  // 今は配列を確保しただけで中身を写していません。これは「深いコピー」ではなく
  // 「別の配列だが中身が違うコピー」です。テストが値の一致を見ています。
  //
  // なお、ここでポインタだけを写す（浅いコピー）ことは unique_ptr が許しません。
  // 生ポインタメンバならコンパイルが通ってしまい、二重解放で落ちます。
}

std::string PulseTrain::name() const
{
  return "PulseTrain";
}

double PulseTrain::sample(std::size_t index) const
{
  return pattern_[index];
}

std::size_t PulseTrain::length() const
{
  return length_;
}

void PulseTrain::set_sample(std::size_t index, double value)
{
  pattern_[index] = value;
}

const std::string & PulseTrain::label() const
{
  return label_;
}

const double * PulseTrain::data() const
{
  return pattern_.get();
}

PulseTrain * PulseTrain::do_clone() const
{
  // TODO: 自分の複製を new して返してください。
  //
  // ヒント: 複製の知識はコピーコンストラクタに既に書いてあります。
  //         ここで書き直さないでください。1 行で済みます。
  //
  // 戻り値の型が PulseTrain *（基底は Waveform *）になっているのが**共変戻り値型**です。
  // std::unique_ptr<PulseTrain> にすると override できません。理由は記事の 6.3。
  return nullptr;
}

SineSweep::SineSweep(double start_hz, double end_hz, std::size_t length)
: start_hz_(start_hz), end_hz_(end_hz), length_(length)
{
  // ここは実装済みです。
}

std::string SineSweep::name() const
{
  return "SineSweep";
}

double SineSweep::sample(std::size_t index) const
{
  const double ratio =
    (length_ <= 1) ? 0.0 : static_cast<double>(index) / static_cast<double>(length_ - 1);
  const double hz = start_hz_ + (end_hz_ - start_hz_) * ratio;
  return std::sin(2.0 * kPi * hz * ratio);
}

std::size_t SineSweep::length() const
{
  return length_;
}

double SineSweep::start_hz() const
{
  return start_hz_;
}

double SineSweep::end_hz() const
{
  return end_hz_;
}

void SineSweep::set_end_hz(double end_hz)
{
  end_hz_ = end_hz;
}

SineSweep * SineSweep::do_clone() const
{
  // TODO: PulseTrain::do_clone() と同じです。
  //
  // SineSweep は値メンバしか持たないので、コピーコンストラクタを自分で書く必要はありません
  // （Rule of Zero）。暗黙のコピーコンストラクタをそのまま使ってください。
  return nullptr;
}

void WaveformLibrary::add(std::unique_ptr<Waveform> waveform)
{
  waveforms_.push_back(std::move(waveform));
}

std::size_t WaveformLibrary::size() const
{
  return waveforms_.size();
}

const Waveform & WaveformLibrary::at(std::size_t index) const
{
  return *waveforms_[index];
}

WaveformLibrary WaveformLibrary::duplicate() const
{
  // TODO: waveforms_ の全要素を clone() して、新しい WaveformLibrary に詰めて返してください。
  //
  // ヒント: for (const std::unique_ptr<Waveform> & waveform : waveforms_) を回して
  //         copy.waveforms_.push_back(waveform->clone()); です。
  //         duplicate() は WaveformLibrary のメンバなので、
  //         copy の private メンバに直接触れます。
  //
  // ここが Prototype の本番です。要素の実体が PulseTrain なのか SineSweep なのかを
  // **一切知らないまま**複製できることを確かめてください。
  //
  // 今は空のライブラリを返しています。
  WaveformLibrary copy;
  return copy;
}
