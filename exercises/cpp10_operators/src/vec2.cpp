// I AM NOT DONE
//
// Vec2 の演算子を実装してください。
// ヘッダ（include/drill/vec2.hpp）の宣言に合わせて、8 つ全部を埋めます。

#include "drill/vec2.hpp"

#include <ostream>

Vec2 & Vec2::operator+=(const Vec2 & other)
{
  // TODO: x と y にそれぞれ足し込み、*this を返す。
  return *this;
}

Vec2 operator+(const Vec2 & a, const Vec2 & b)
{
  // TODO: 成分ごとに足す。
  return Vec2{};
}

Vec2 operator-(const Vec2 & a, const Vec2 & b)
{
  // TODO: 成分ごとに引く。
  return Vec2{};
}

Vec2 operator*(const Vec2 & v, double s)
{
  // TODO: 各成分を s 倍する。
  return Vec2{};
}

Vec2 operator*(double s, const Vec2 & v)
{
  // TODO: 上の関数に委譲すれば 1 行で書けます。
  return Vec2{};
}

bool operator==(const Vec2 & a, const Vec2 & b)
{
  // TODO: x と y の両方が等しいか。
  return false;
}

bool operator!=(const Vec2 & a, const Vec2 & b)
{
  // TODO: operator== を使って書く。論理を 2 箇所に書かないこと。
  return false;
}

bool operator<(const Vec2 & a, const Vec2 & b)
{
  // TODO: length_squared() の小さい順。
  //       <= ではなく < を使うこと（<= だと厳密弱順序を壊し、std::sort が未定義動作になる）。
  return false;
}

std::ostream & operator<<(std::ostream & os, const Vec2 & v)
{
  // TODO: "(1, 2)" の形で os に書き込み、os を返す。
  //       return os; を忘れると << が繋げられなくなります。
  return os;
}
