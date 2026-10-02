# cppb08 inline / explicit / constexpr / trailing const [C++ Basics]

In this exercise you add four qualifiers to declarations by hand.

## In this exercise only, the file you edit is a header

You edit `include/drill/qualifiers.hpp`, not a file in `src/`.

`inline`, `explicit`, `constexpr`, and trailing `const` are **all qualifiers that go on a declaration**.
You cannot add them later to the definition in the `.cpp` file.

```cpp
// This does not work
explicit Pair::Pair(int a, int b) { }   // error: ‘explicit’ outside class declaration
```

That is why the file you edit for these four qualifiers is the header.

## What to do

Fill in TODO(1) to (4) in `include/drill/qualifiers.hpp`.

| TODO | Qualifier to add | What happens without it |
| --- | --- | --- |
| (1) | `explicit` | `Meters m = 3.0;` compiles |
| (2) | trailing `const` | you cannot call `value()` on a `const Meters` |
| (3) | `constexpr` | `static_assert(square(5) == 25)` does not compile |
| (4) | `inline` | each of the two translation units gets its own copy, giving `multiple definition` |

## Run it

```bash
./drill run cppb08
```

**While you have not started, the build does not pass.** All four qualifiers in this exercise are
compile-time properties, so they do not show up as "a test turns red". They show up as "compiling and linking stop".
Read each message one by one and match it to the TODO it is about.

| Message | Matching TODO |
| --- | --- |
| `static assertion failed: Meters のコンストラクタに explicit を…` (the message text is in Japanese: "add explicit to the Meters constructor ...") | (1) |
| `passing ‘const Meters’ as ‘this’ argument discards qualifiers` | (2) |
| `non-constant condition for static assertion` | (3) |
| `multiple definition of ‘twice(int)’` | (4) |

(4) shows up at link time, not at compile time. You cannot see it until you fix the other three.

## Common pitfalls

- `explicit` **works on a constructor with one argument**. A one-argument constructor such as `Meters(double)`
  becomes a path for implicit type conversion if you do not add it.
- `constexpr` includes `inline`. So once you fix (3), `square` is not defined multiple times.
  That is why only `twice` needs `inline`.
- Even with `constexpr`, you can still call the function at run time. It means "can be evaluated at compile time **too**".

## Tests

| Test | What it checks |
| --- | --- |
| `Explicitが暗黙変換を止める` (explicit stops the implicit conversion) | whether `std::is_convertible_v<double, Meters>` is false |
| `Const関数はConstオブジェクトから呼べる` (a const function can be called from a const object) | trailing `const` |
| `Constexprはコンパイル時に評価される` (constexpr is evaluated at compile time) | whether `static_assert` passes |
| `Inlineで多重定義を避ける` (inline avoids multiple definitions) | whether two translation units can be linked |

## References

- [8. Other qualifiers](../../docs-en/cpp-basics/08_other_qualifiers.md)
