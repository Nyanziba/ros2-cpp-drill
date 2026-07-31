// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <iosfwd>

/// 2 次元ベクトル。
///
/// 対称な 2 項演算子（+ - * == != <）は、あえて「自由関数」として宣言しています。
/// メンバ関数にすると 2.0 * v（左辺が double）が書けなくなるからです。
/// 詳しくは docs/cpp/10_演算子オーバーロード.md の 10.2 節。
struct Vec2
{
  double x = 0.0;
  double y = 0.0;

  /// 原点からの距離の 2 乗。平方根を取らないので誤差が乗りません。
  double length_squared() const { return x * x + y * y; }

  /// 加算代入。これだけはメンバとして宣言します（言語の作法）。
  /// 自分自身への参照を返すこと。
  Vec2 & operator+=(const Vec2 & other);
};

// --- 自由関数として実装するもの ---

Vec2 operator+(const Vec2 & a, const Vec2 & b);
Vec2 operator-(const Vec2 & a, const Vec2 & b);

/// スカラー倍。**左右の順番ごとに別の関数が必要**です。
Vec2 operator*(const Vec2 & v, double s);
Vec2 operator*(double s, const Vec2 & v);

bool operator==(const Vec2 & a, const Vec2 & b);
bool operator!=(const Vec2 & a, const Vec2 & b);

/// length_squared() の小さい順。std::sort が使います。
/// 厳密弱順序（a < a が必ず false）を守ること。
bool operator<(const Vec2 & a, const Vec2 & b);

/// "(x, y)" の形で出力する。std::ostream & を返すこと（<< を繋げるため）。
std::ostream & operator<<(std::ostream & os, const Vec2 & v);
