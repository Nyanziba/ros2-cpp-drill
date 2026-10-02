# 2. Fixed-width integers and implicit conversions

> **Goal of this chapter**: On microcontrollers you use integer types with a "fixed number of bytes", such as `uint8_t` and `int16_t`. The size of the standard `int` and `char` differs between implementations, but a fixed-width integer is **always that size**. Also, **integer promotion** from a small type to a larger type, and wraparound on overflow of unsigned types, cannot be avoided in microcontroller control. If you understand them well in this chapter, you will make fewer mistakes later when you implement electrical control drivers.

## 2.1 Fixed-width integer types — `uint8_t`, `int16_t`, `uint32_t`

In standard C, the sizes of `int`, `short`, and `long` are not guaranteed.

```c
int a = 0;      // size unknown (implementation-dependent)
short b = 0;    // size unknown
long c = 0;     // size unknown
```

Depending on the implementation, they may be 16 bits, 32 bits, or 64 bits.

**Fixed-width integer types (C99 and later)**

The types defined in `<stdint.h>` have a definite size.

```c
#include <stdint.h>

uint8_t u8 = 255;      // always 1 byte (0 to 255)
int16_t s16 = -1000;   // always 2 bytes (-32768 to 32767)
uint32_t u32 = 4294967295U;  // always 4 bytes (0 to 4294967295)
int64_t s64 = 9223372036854775807LL;  // always 8 bytes
```

Microcontroller registers and memory layouts are fixed, so **it is safer to use fixed-width integer types**.

## 2.2 Signed and unsigned

**Unsigned type (unsigned)**
Only values of 0 or more. All bits are used for the value.

```c
uint8_t u = 255;  // maximum value
```

**Signed type (signed)**
It can also represent negative numbers. The top bit shows the sign (two's complement).

```c
int8_t s = -1;  // the minimum value is -128
```

Even with the same number of bits, an unsigned type can represent larger positive values.

| Type | Size | Minimum | Maximum |
|---|---|---|---|
| `int8_t` | 1 byte | -128 | 127 |
| `uint8_t` | 1 byte | 0 | 255 |
| `int16_t` | 2 bytes | -32768 | 32767 |
| `uint16_t` | 2 bytes | 0 | 65535 |

## 2.3 Integer promotion

When you use a value of a small type, in many cases it is **automatically widened to a larger type**.

```c
uint8_t a = 200;
uint8_t b = 100;
int result = a + b;  // a and b are promoted to int and then added
```

`a` and `b` are `uint8_t`, but they are widened to `int` before the addition.
As a result, `result` is 300 (the correct value).

**Note: operations between `uint8_t` values are also promoted**

```c
uint8_t a = 200;
uint8_t b = 100;
uint8_t c = a + b;  // 300 (computed as int) → converted back to uint8_t gives 44
```

The right side is computed as `int` and gives 300. But when it is converted back to `uint8_t` on the left side, only the low 8 bits are taken, so the result is 44.

(The remainder of division by 256 is stored. `300 % 256 = 44`)

## 2.4 Overflow and unsigned wraparound

**Unsigned type: defined behavior (wraparound)**

```c
uint8_t u = 255;
u++;  // overflow
printf("%u\n", u);  // output: 0
```

Adding 1 to 255 gives 256, but `uint8_t` only goes up to 255, so it **goes back to 0** (wraparound).
This is **defined behavior**, and you can rely on it.

In microcontroller control, this wraparound is sometimes used to make counters and timers.

**Signed type: undefined behavior (dangerous)**

```c
int8_t s = 127;
s++;  // overflow
printf("%d\n", s);  // result is undefined (often -128, but not guaranteed)
```

Overflow of a signed type is **undefined behavior**, and the compiler is allowed to do anything.
It usually becomes -128, but you must not rely on that.

## 2.5 The signedness of `char` is implementation-dependent

Whether the `char` type is signed or unsigned is left to the implementation.

```c
char c = 255;
if (c > 0) {
  printf("positive\n");  // not certain whether it takes this path or the one below
} else {
  printf("negative\n");
}
```

If you want to state the sign for certain, use `signed char` or `unsigned char`.

```c
signed char sc = -1;    // always signed
unsigned char uc = 255; // always unsigned
```

This is why using `uint8_t` and `int8_t` is recommended in microcontroller code.

## 2.6 Check the size of a type with `sizeof`

Fixed-width integer types are guaranteed to have the size in their names, but you can check to be sure.

```c
printf("sizeof(uint8_t) = %zu\n", sizeof(uint8_t));    // 1
printf("sizeof(uint16_t) = %zu\n", sizeof(uint16_t));  // 2
printf("sizeof(uint32_t) = %zu\n", sizeof(uint32_t));  // 4
printf("sizeof(uint64_t) = %zu\n", sizeof(uint64_t));  // 8
```

For the format specifier, use `%zu` (the size type).

## Try it yourself

Check the behavior of fixed-width integer types, overflow, and integer promotion.

```c
// ch02_handson.c
#include <stdio.h>
#include <stdint.h>

int main()
{
  printf("== Fixed-width integers and overflow ==\n");

  uint8_t small = 255;
  printf("uint8_t: %u (max)\n", small);
  small++;
  printf("After ++: %u (wrapped to 0)\n", small);

  printf("\n== Integer promotion ==\n");
  uint8_t a = 200;
  uint8_t b = 100;
  printf("a = %u, b = %u\n", a, b);
  printf("a + b = %d (promoted to int)\n", a + b);

  printf("\n== Size of fixed-width types ==\n");
  printf("uint8_t: %zu bytes\n", sizeof(uint8_t));
  printf("uint16_t: %zu bytes\n", sizeof(uint16_t));
  printf("uint32_t: %zu bytes\n", sizeof(uint32_t));
  printf("int8_t: %zu bytes\n", sizeof(int8_t));
  printf("int16_t: %zu bytes\n", sizeof(int16_t));

  printf("\n== Signedness matters ==\n");
  int8_t neg = -1;
  uint8_t uns = 255;
  printf("int8_t(-1) vs uint8_t(255)\n");
  printf("As int: %d vs %d\n", neg, uns);
  printf("Equal? %s\n", neg == (int8_t)uns ? "yes" : "no");

  return 0;
}
```

**Predict**: Does a `uint8_t` go back to 0 when you increment it? Is the result of `a + b` 300? Are the `sizeof` values 1, 2, 4?

Compile and run:

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic ch02_handson.c -o ch02_handson && ./ch02_handson
```

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
== Fixed-width integers and overflow ==
uint8_t: 255 (max)
After ++: 0 (wrapped to 0)

== Integer promotion ==
a = 200, b = 100
a + b = 300 (promoted to int)

== Size of fixed-width types ==
uint8_t: 1 bytes
uint16_t: 2 bytes
uint32_t: 4 bytes
int8_t: 1 bytes
int16_t: 2 bytes

== Signedness matters ==
int8_t(-1) vs uint8_t(255)
As int: -1 vs 255
Equal? yes
```

</details>

Look closely at why you get `Equal? yes`. The `As int:` line shows
different values, `-1 vs 255`, but the comparison is done with `neg == (int8_t)uns`.
`(int8_t)255` becomes `-1`, so they match.

**However, the result of converting an out-of-range value to a signed type is "implementation-defined".**
It is not undefined behavior, but the standard does not fix the value, so it is not portable.
On gcc / x86-64 it wraps around in two's complement and becomes `-1`,
but do not write code that relies on this.

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. Adding 1 to 255 goes back to 0 (unsigned wraparound)
2. `a + b` is computed after promotion, so it is 300
3. Checking with `sizeof`, `uint8_t` is 1 byte and `uint16_t` is 2 bytes, and so on, as predicted

</details>

## Common pitfalls

**`error: format '%d' expects argument of type 'int', but argument 2 has type 'uint32_t'`**
The format specifier is wrong when printing a fixed-width integer type.
Use `%u` for `uint8_t` and `uint16_t`, and `%d` for `int8_t` and `int16_t`.
In C99 you can also use the portable macros `PRId8` and `PRIu8` from `inttypes.h`.

**The calculation result is strange (an operation between small types gives an unexpected value)**
It may be caused by integer promotion.
Check whether, with `uint8_t x = 255; uint8_t y = x + 1;`, y is not 0 but 256 (or a strange value).

**Assigning a negative number to a `uint8_t` does not look negative**
It is an unsigned type, so a negative value is read as a large positive value.
With `uint8_t u = -1;`, `u` becomes 255.

**No compiler warning on overflow**
With GCC, enabling `-Woverflow` gives a warning.
But real overflow happens at run time, so checking the behavior is important.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c02_fixed_width_int` — fixed-width integers and implicit conversions

```bash
./drill run c02
```

If you get stuck, use `./drill hint c02`. From the exercise side, `./drill read c02` brings you back to this chapter.

## References

- C99 standard - [Integer types](https://www.open-std.org/JTC1/SC22/WG14/www/docs/n1570.pdf) (PDF, see 6.2.5)
- [cppreference: fixed width integer types](https://en.cppreference.com/w/c/types/integer)
- [ROS 2 coding conventions](../ros2_coding_conventions.md) — how to choose data types

---

Previous chapter → [1. Separate compilation and header guards](01_separate_compilation_and_header_guards.md)

Next chapter → [3. Pointers 1 — addresses and dereferencing](03_pointers_1.md)
