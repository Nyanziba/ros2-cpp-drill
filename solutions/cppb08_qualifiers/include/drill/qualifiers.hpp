#pragma once

class Meters
{
public:
  // (1) explicit。double から Meters への暗黙変換を止めます。
  explicit Meters(double value)
  : value_(value)
  {
  }

  // (2) 末尾 const。const Meters からも呼べるようになります。
  double value() const
  {
    return value_;
  }

private:
  double value_;
};

// (3) constexpr。コンパイル時にも実行時にも評価できます。
// constexpr は inline を含むので、これで多重定義にもなりません。
constexpr int square(int x)
{
  return x * x;
}

// (4) inline。複数の翻訳単位から include されても実体が 1 つに畳まれます。
inline int twice(int x)
{
  return x * 2;
}

int use_twice(int x);
