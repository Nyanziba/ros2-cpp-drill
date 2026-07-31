// I AM NOT DONE
//
// この課題で編集するのは「このヘッダ」です。
// inline / explicit / constexpr / 末尾 const は、どれも宣言に付ける修飾子なので、
// .cpp 側に後付けすることができません。だから編集対象がヘッダになります。
#pragma once

class Meters
{
public:
  // TODO(1) 暗黙変換を禁止してください。
  // いまは double が勝手に Meters に化けます。
  //   Meters m = 3.0;   // ← これが通ってしまう
  // テストは std::is_convertible でこれを見ています。
  Meters(double value)
  : value_(value)
  {
  }

  // TODO(2) const オブジェクトからも呼べるようにしてください（末尾 const）。
  double value()
  {
    return value_;
  }

private:
  double value_;
};

// TODO(3) コンパイル時に評価できるようにしてください。
// テストが static_assert(square(5) == 25) を書いているので、
// 実行時関数のままでは「non-constant condition」でコンパイルが止まります。
int square(int x)
{
  return x * x;
}

// TODO(4) このヘッダは src/qualifiers.cpp と test/test_exercise.cpp の
// 両方から include されます。いまのままだと 2 つの翻訳単位にそれぞれ実体ができて、
// リンク時に multiple definition になります。1 つだけ足りない修飾子があります。
int twice(int x)
{
  return x * 2;
}

// src/qualifiers.cpp 側で定義されている関数（あちらは編集しません）。
// twice() をもう 1 つの翻訳単位から呼ぶために置いてあります。
int use_twice(int x);
