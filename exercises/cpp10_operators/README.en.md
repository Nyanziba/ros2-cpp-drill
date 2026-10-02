# c08 Overload operators [C++]

## What to do

Implement **8 operators** in `src/vec2.cpp`. For Vec2 (a 2D vector), you implement addition, subtraction, scalar multiplication, comparison, and stream output.

| Operator | Implemented as | Description |
| --- | --- | --- |
| `operator+` | free function | `a + b` adds two vectors |
| `operator-` | free function | `a - b` subtracts two vectors |
| `operator*` (both left and right) | free function | scalar multiplication with both `v * s` and `s * v` |
| `operator+=` | member function | `a += b` adds into itself and returns a reference to itself |
| `operator==` | free function | `a == b` tests equality |
| `operator!=` | free function | `a != b` tests inequality (implement it using `operator==`) |
| `operator<` | free function | `a < b` compares the size. Compare by the value of `length_squared()` |
| `operator<<` | free function | `std::cout << v` prints in the form `(x, y)` |

**Important: implement the symmetric operators (`+`, `-`, `*`, `==`, `!=`, `<`) as free functions.** With a member function you cannot write `2.0 * v`. The left side is a `double`, so it would have to be a member function of `double`, and that is not possible. See section 10.2 of [10. Operator overloading](../../docs-en/cpp/10_operator_overloading.md) for details.

## Run it

```bash
./drill run c08
```

When all 8 tests pass, you have cleared the exercise.

## Common pitfalls

**You must write two `operator*`**

`Vec2 * double` and `double * Vec2` are different functions.

```cpp
Vec2 operator*(const Vec2 & v, double s) { ... }
Vec2 operator*(double s, const Vec2 & v) { ... }
```

If you write it as a member function, you cannot write the case where the left side is a `double`. The test `スカラー倍は左右どちらの順番でも書ける` (scalar multiplication can be written in either order) requires both.

**`operator<` uses `<`, not `<=`**

If you use `<=` in `operator<` to mean "less than or equal", `std::sort` gets undefined behavior in the test `大小比較は厳密弱順序である` (the comparison is a strict weak ordering). For the reason, see section 10.3 of [10. Operator overloading](../../docs-en/cpp/10_operator_overloading.md).

```cpp
bool operator<(const Vec2 & a, const Vec2 & b) {
  return a.length_squared() < b.length_squared();  // use < , not <=
}
```

**`operator<<` must return `std::ostream &`, or you cannot chain `<<`**

```cpp
std::ostream & operator<<(std::ostream & os, const Vec2 & v) {
  os << "(" << v.x << ", " << v.y << ")";
  return os;  // Always write this. Without it, you cannot chain <<
}
```

The test `ostream演算子は繋げられる` (the ostream operator can be chained) keeps writing `oss << "a=" << Vec2{1.0, 2.0} << " b=" << Vec2{3.0, 4.0}`, so each `<<` must return `std::ostream &`.

**`operator+=` returns `Vec2 &`, and it returns `*this`, not a move**

```cpp
Vec2 & Vec2::operator+=(const Vec2 & other) {
  x += other.x;
  y += other.y;
  return *this;  // Add the &, so you return a reference with the same address
}
```

Return a reference, not a copy. The test `加算代入は自分自身への参照を返す` (the compound assignment returns a reference to itself) checks this by comparing the addresses with `EXPECT_EQ(&returned, &a)`.

**Write `operator!=` using `operator==`**

```cpp
bool operator!=(const Vec2 & a, const Vec2 & b) {
  return !(a == b);
}
```

If `operator==` is implemented correctly, `!=` is automatically correct. Do not write the same logic in two places.

## Tests

```bash
./drill run c08
```

| Test | What it checks |
| --- | --- |
| `足し算と引き算` (addition and subtraction) | the implementation of `operator+` and `operator-` |
| `スカラー倍は左右どちらの順番でも書ける` (scalar multiplication can be written in either order) | whether `operator*` works on both sides (`v * 2.0` and `2.0 * v`) |
| `加算代入は自分自身への参照を返す` (the compound assignment returns a reference to itself) | whether `operator+=` returns `Vec2 &` and returns the same address |
| `等価比較と非等価比較` (equality and inequality comparison) | the implementation of `operator==` and `operator!=`, and whether a difference in either component is detected |
| `大小比較は厳密弱順序である` (the comparison is a strict weak ordering) | whether `operator<` uses `<`, `a < a` is false, and both are false when the lengths are the same |
| `std_sortで並べられる` (you can sort with std::sort) | whether `operator<` works correctly with `std::sort` |
| `ostreamに流せる` (you can stream to an ostream) | whether `operator<<` prints in the form `(x, y)` |
| `ostream演算子は繋げられる` (the ostream operator can be chained) | whether `operator<<` returns `std::ostream &` and `<<` can be chained |

## References

- [10. Operator overloading](../../docs-en/cpp/10_operator_overloading.md) — section 10.2 (member vs free function), section 10.3 (comparison operators)
- [cppreference: Operator overloading](https://en.cppreference.com/w/cpp/language/operators)
