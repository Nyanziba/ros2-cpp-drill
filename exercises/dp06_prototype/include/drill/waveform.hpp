// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

/// 円周率。<cmath> の M_PI は規格の外なので自前で置きます。
constexpr double kPi = 3.14159265358979323846;

/// モータに流す波形パターンの基底クラス。
///
/// 【なぜ clone() が要るのか】
/// C++ には**コピーコンストラクタがあります**。Java の `Cloneable` / `clone()` は
/// 「Java にコピーの言語機能が無いから」必要だったものです。
/// つまり `PulseTrain a = b;` で済む場面では clone() は要りません。
///
/// clone() が本当に要るのは **多態のとき**だけです。
/// `std::unique_ptr<Waveform>` しか持っていない状態で、実体が PulseTrain なのか
/// SineSweep なのかを知らないまま複製したい ―― このときだけです。
///
/// 【NVI との組み合わせ】
/// public な clone() は **非仮想**、複製の中身は private 仮想の do_clone() です。
/// do_clone() が生ポインタを返しているのは飾りではありません。
/// 生ポインタなら **共変戻り値型**（Derived * で override できる）が使えますが、
/// `std::unique_ptr<Base>` → `std::unique_ptr<Derived>` は共変になれないからです。
/// 詳しくは docs/patterns/06_Prototype.md の 6.3。
class Waveform
{
public:
  virtual ~Waveform() = default;

  /// 波形の種類名。複製で型が保たれたかを見るために使います。
  virtual std::string name() const = 0;

  /// index 番目のサンプル値。範囲検査はしません（呼ぶ側の責任）。
  virtual double sample(std::size_t index) const = 0;

  /// サンプル点の数。
  virtual std::size_t length() const = 0;

  /// 自分と同じ型・同じ状態の複製を返す。
  ///
  /// **非仮想です。** override しないでください。
  /// 中身は private 仮想の do_clone() に委譲します（NVI）。
  std::unique_ptr<Waveform> clone() const;

protected:
  Waveform() = default;

  /// 派生クラスのコピーコンストラクタから呼ばれます。
  /// public にすると `Waveform w = pulse_train;` が書けてしまい、**スライシング**します。
  /// protected にしておくと、その行がコンパイルエラーになります。
  Waveform(const Waveform &) = default;

  /// 基底への代入はスライシングそのものなので禁止します。
  Waveform & operator=(const Waveform &) = delete;

private:
  /// 複製の実体。**共変戻り値型**にするため、あえて生ポインタを返します。
  /// 呼ぶのは clone() だけで、clone() が即座に unique_ptr に包みます。
  virtual Waveform * do_clone() const = 0;
};

/// パルス列。サンプル値を**ヒープ上の配列で持つ**ので、複製は深いコピーが要ります。
///
/// std::unique_ptr<double[]> をメンバに持っているため、
/// **暗黙のコピーコンストラクタは自動的に delete されます**。
/// 深いコピーが欲しければ自分で書くしかありません。
class PulseTrain : public Waveform
{
public:
  /// 全サンプルを 0 で初期化して作ります。
  PulseTrain(std::string label, std::size_t length);

  /// 深いコピー。**自分で書きます**（src/waveform.cpp）。
  PulseTrain(const PulseTrain & other);

  /// 代入は要らないので禁止します（Rule of Five を「全部禁止」で満たす形）。
  PulseTrain & operator=(const PulseTrain &) = delete;

  ~PulseTrain() override = default;

  std::string name() const override;
  double sample(std::size_t index) const override;
  std::size_t length() const override;

  void set_sample(std::size_t index, double value);

  /// このパルス列の識別名。
  const std::string & label() const;

  /// 内部バッファの先頭アドレス。
  /// 「複製がバッファを共有していないこと」をテストから確かめるために公開しています。
  const double * data() const;

private:
  PulseTrain * do_clone() const override;

  std::string label_;
  std::unique_ptr<double[]> pattern_;
  std::size_t length_;
};

/// 周波数を掃引する正弦波。**値メンバしか持ちません**。
///
/// だからコピーコンストラクタもデストラクタも書きません（Rule of Zero）。
/// コンパイラが作る暗黙のコピーで正しく複製できます。
/// do_clone() だけが必要です。
class SineSweep : public Waveform
{
public:
  SineSweep(double start_hz, double end_hz, std::size_t length);

  std::string name() const override;
  double sample(std::size_t index) const override;
  std::size_t length() const override;

  double start_hz() const;
  double end_hz() const;
  void set_end_hz(double end_hz);

private:
  SineSweep * do_clone() const override;

  double start_hz_;
  double end_hz_;
  std::size_t length_;
};

/// 波形のひな型置き場。「登録しておいた原型を複製して使う」という Prototype の使い方そのもの。
///
/// 【なぜコピーを明示的に delete しているのか】
/// メンバは std::vector<std::unique_ptr<Waveform>> です。
/// unique_ptr はコピーできないので「コンパイラが暗黙のコピーを delete してくれる」と
/// 思いがちですが、**そうなりません**。std::vector はコピーコンストラクタを
/// 宣言だけはしていて、実際に使おうとしたときに初めてエラーになるからです。
/// そのため std::is_copy_constructible<WaveformLibrary> は true になってしまいます。
/// 「コピー禁止」を型の性質として表明したいなら、自分で = delete と書きます。
///
/// ムーブはできます。丸ごと複製したいなら duplicate() を使います。
class WaveformLibrary
{
public:
  WaveformLibrary() = default;

  WaveformLibrary(const WaveformLibrary &) = delete;
  WaveformLibrary & operator=(const WaveformLibrary &) = delete;

  // コピーを user-declared にするとムーブが暗黙に生成されなくなります。
  // Rule of Five —— 1 つ書いたら残りも意図を書く。
  WaveformLibrary(WaveformLibrary &&) = default;
  WaveformLibrary & operator=(WaveformLibrary &&) = default;

  void add(std::unique_ptr<Waveform> waveform);

  std::size_t size() const;

  /// index 番目のひな型。範囲検査はしません。
  const Waveform & at(std::size_t index) const;

  /// 全要素を clone した新しいライブラリを返す。
  WaveformLibrary duplicate() const;

private:
  std::vector<std::unique_ptr<Waveform>> waveforms_;
};
