// 解答例
//
// 結城本 第6章 Prototype を C++ で書いたもの。
// 要点は 3 つです。
//   1. 多態でないなら clone() は要らない。コピーコンストラクタで足りる
//   2. do_clone() は生ポインタを返す（共変戻り値型）。clone() が unique_ptr に包む
//   3. unique_ptr メンバを持つ型は、深いコピーを自分で書く

#include "drill/waveform.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

std::unique_ptr<Waveform> Waveform::clone() const
{
  // do_clone() の戻り値は実際には PulseTrain * や SineSweep * ですが、
  // ここでは Waveform * として受け取ります。unique_ptr に包むのはこの 1 か所だけです。
  return std::unique_ptr<Waveform>(do_clone());
}

PulseTrain::PulseTrain(std::string label, std::size_t length)
: label_(std::move(label)), pattern_(new double[length]()), length_(length)
{
}

PulseTrain::PulseTrain(const PulseTrain & other)
: Waveform(other),                          // 基底の protected なコピーコンストラクタ
  label_(other.label_),
  pattern_(new double[other.length_]()),    // 新しい配列を確保する = 深いコピー
  length_(other.length_)
{
  // ポインタをコピーするのではなく、中身を写します。
  // ここを忘れると「複製したのに元を変えると複製も変わる」浅いコピーになります。
  std::copy(other.pattern_.get(), other.pattern_.get() + other.length_, pattern_.get());
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
  // 戻り値が PulseTrain *（基底は Waveform *）。これが共変戻り値型です。
  // 中身は自分のコピーコンストラクタに任せます。複製の知識を 2 か所に書きません。
  return new PulseTrain(*this);
}

SineSweep::SineSweep(double start_hz, double end_hz, std::size_t length)
: start_hz_(start_hz), end_hz_(end_hz), length_(length)
{
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
  // SineSweep は値メンバしか持たないので、コピーコンストラクタは書いていません
  // （Rule of Zero）。暗黙のコピーコンストラクタがそのまま正しく動きます。
  return new SineSweep(*this);
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
  // ここが Prototype の本番です。
  // 要素の実体の型（PulseTrain なのか SineSweep なのか）を一切知らないまま複製できます。
  WaveformLibrary copy;
  copy.waveforms_.reserve(waveforms_.size());
  for (const std::unique_ptr<Waveform> & waveform : waveforms_) {
    copy.waveforms_.push_back(waveform->clone());
  }
  return copy;
}
