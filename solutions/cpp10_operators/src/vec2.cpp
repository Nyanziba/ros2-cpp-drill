#include "drill/vec2.hpp"

#include <ostream>

Vec2 & Vec2::operator+=(const Vec2 & other)
{
  x += other.x;
  y += other.y;
  return *this;              // これがあるから v1 += v2 += v3 やチェーンが書ける
}

Vec2 operator+(const Vec2 & a, const Vec2 & b)
{
  return Vec2{a.x + b.x, a.y + b.y};
}

Vec2 operator-(const Vec2 & a, const Vec2 & b)
{
  return Vec2{a.x - b.x, a.y - b.y};
}

Vec2 operator*(const Vec2 & v, double s)
{
  return Vec2{v.x * s, v.y * s};
}

Vec2 operator*(double s, const Vec2 & v)
{
  return v * s;              // 上のオーバーロードに委譲する
}

bool operator==(const Vec2 & a, const Vec2 & b)
{
  return a.x == b.x && a.y == b.y;
}

bool operator!=(const Vec2 & a, const Vec2 & b)
{
  return !(a == b);          // == の裏返し。論理を 2 箇所に書かない
}

bool operator<(const Vec2 & a, const Vec2 & b)
{
  // < を使う。<= にすると a == b のとき a<b と b<a が両方 true になり、
  // 厳密弱順序が壊れて std::sort が未定義動作になる。
  return a.length_squared() < b.length_squared();
}

std::ostream & operator<<(std::ostream & os, const Vec2 & v)
{
  os << "(" << v.x << ", " << v.y << ")";
  return os;                 // 参照を返すから std::cout << a << b と繋げられる
}
