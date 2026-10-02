# 6. Structs and alignment

> **Goal of this chapter**: The reason `sizeof` is not simply "the sum of the member sizes" is memory alignment. For the CPU to access data efficiently, each data type has a required start address. As a result, the order of the members changes the size of the whole struct. This knowledge is a must when you handle microcontroller registers or network protocols.

## 6.1 The order of members changes `sizeof`

**The position in memory is decided by the type. Because of each type's alignment requirement, filler bytes (padding) are inserted in between.**

```c
struct unordered {
    char a;      // 1 byte
    int b;       // 4 bytes (4-byte aligned)
    char c;      // 1 byte
};
```

`int` requires 4-byte alignment, so 3 bytes of "filler" are inserted right after `a`.

Measured values:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// unordered_layout.c
#include <stdio.h>
#include <stddef.h>

struct unordered {
    char a;
    int b;
    char c;
};

int main(void)
{
    printf("struct unordered:  sizeof = %zu\n", sizeof(struct unordered));
    printf("  offsetof(a) = %zu\n", offsetof(struct unordered, a));
    printf("  offsetof(b) = %zu\n", offsetof(struct unordered, b));
    printf("  offsetof(c) = %zu\n", offsetof(struct unordered, c));
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic unordered_layout.c -o unordered_layout && ./unordered_layout
```

</details>

<!-- measure: -->
```
struct unordered:  sizeof = 12
  offsetof(a) = 0
  offsetof(b) = 4
  offsetof(c) = 8
```

Memory layout:

```
[0]a [1][2][3] padding [4-7]b [8]c [9][10][11] padding
```

**Just reordering the same members shrinks the size.**

```c
struct reordered {
    int b;       // 4 bytes (4-byte aligned at offset 0)
    char a;      // 1 byte
    char c;      // 1 byte  (fits at offset 5)
};
```

Measured values:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// reordered_layout.c
#include <stdio.h>
#include <stddef.h>

struct reordered {
    int b;
    char a;
    char c;
};

int main(void)
{
    printf("struct reordered:  sizeof = %zu\n", sizeof(struct reordered));
    printf("  offsetof(b) = %zu\n", offsetof(struct reordered, b));
    printf("  offsetof(a) = %zu\n", offsetof(struct reordered, a));
    printf("  offsetof(c) = %zu\n", offsetof(struct reordered, c));
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic reordered_layout.c -o reordered_layout && ./reordered_layout
```

</details>

<!-- measure: -->
```
struct reordered:  sizeof = 8
  offsetof(b) = 0
  offsetof(a) = 4
  offsetof(c) = 5
```

Memory layout:

```
[0-3]b [4]a [5]c [6][7] padding
```

The members are the same, but `unordered` is 12 bytes and `reordered` is 8 bytes. **The order matters.**

## 6.2 `offsetof` and `_Alignof`

**`offsetof(struct_name, member_name)` returns the start position of a member in bytes.**

```c
#include <stddef.h>

struct unordered {
    char a;
    int b;
    char c;
};

offsetof(struct unordered, a);  // 0
offsetof(struct unordered, b);  // 4
offsetof(struct unordered, c);  // 8
```

**`_Alignof(type)` returns the alignment requirement of a type.**

Measured values:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// alignof_demo.c
#include <stdio.h>

struct unordered {
    char a;
    int b;
    char c;
};

int main(void)
{
    printf("_Alignof(char) = %zu\n", _Alignof(char));
    printf("_Alignof(int) = %zu\n", _Alignof(int));
    printf("_Alignof(struct unordered) = %zu\n", _Alignof(struct unordered));
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic alignof_demo.c -o alignof_demo && ./alignof_demo
```

</details>

<!-- measure: filter="tail -n 3" -->
```
_Alignof(char) = 1
_Alignof(int) = 4
_Alignof(struct unordered) = 4
```

The alignment requirement of a struct is "the alignment requirement of its largest member". `int` has a 4-byte alignment requirement, so the whole struct is also 4-byte.

## 6.3 `#pragma pack` and `__attribute__((packed))` — removing padding

**If you want to place the members tightly with no padding, use `__attribute__((packed))`.**

```c
struct packed_attr {
    char a;
    int b;
    char c;
} __attribute__((packed));
```

Measured values:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// packed_attr.c
#include <stdio.h>
#include <stddef.h>

struct packed_attr {
    char a;
    int b;
    char c;
} __attribute__((packed));

int main(void)
{
    printf("struct packed_attr: sizeof = %zu\n", sizeof(struct packed_attr));
    printf("  offsetof(a) = %zu\n", offsetof(struct packed_attr, a));
    printf("  offsetof(b) = %zu\n", offsetof(struct packed_attr, b));
    printf("  offsetof(c) = %zu\n", offsetof(struct packed_attr, c));
    printf("  _Alignof = %zu\n", _Alignof(struct packed_attr));
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic packed_attr.c -o packed_attr && ./packed_attr
```

</details>

<!-- measure: filter="tail -n 5" -->
```
struct packed_attr: sizeof = 6
  offsetof(a) = 0
  offsetof(b) = 1
  offsetof(c) = 5
  _Alignof = 1
```

Memory layout:

```
[0]a [1-4]b [5]c
```

The size shrank from 12 to 6. This is because the padding is gone.

**Warning: the danger of unaligned access.** On many microcontrollers, if a 4-byte value is placed at an address that is not 4-byte aligned, access has a penalty or crashes. Use `packed` only where it is surely safe, such as a network protocol (converting data received as a byte sequence into a struct).

```c
// example for a network protocol (safe)
struct CAN_message {
    uint32_t id;      // network byte order
    uint8_t dlc;
    uint8_t data[8];
} __attribute__((packed));
```

## 6.4 Optimize by arranging the member order

**Large types first, small types after.**

```c
// bad order: 12 bytes
struct bad {
    char a;
    int b;
    char c;
    short d;
};

// good order: 8 bytes
struct good {
    int b;
    short d;
    char a;
    char c;
};
```

When you want to save memory, or when you think about cache efficiency, it is worth organizing the order of the members.

## Try it yourself

Check padding, alignment, `offsetof`, and `packed` for yourself.

```c
// struct_all.c
#include <stdio.h>
#include <stddef.h>

struct unordered {
    char a;
    int b;
    char c;
};

struct reordered {
    int b;
    char a;
    char c;
};

struct packed_example {
    char a;
    int b;
    char c;
} __attribute__((packed));

int main(void)
{
    printf("== Padding and sizeof ==\n");
    printf("struct unordered:  sizeof = %zu\n", sizeof(struct unordered));
    printf("  offsetof(a) = %zu\n", offsetof(struct unordered, a));
    printf("  offsetof(b) = %zu\n", offsetof(struct unordered, b));
    printf("  offsetof(c) = %zu\n", offsetof(struct unordered, c));

    printf("\nstruct reordered:  sizeof = %zu\n", sizeof(struct reordered));
    printf("  offsetof(b) = %zu\n", offsetof(struct reordered, b));
    printf("  offsetof(a) = %zu\n", offsetof(struct reordered, a));
    printf("  offsetof(c) = %zu\n", offsetof(struct reordered, c));

    printf("\n== _Alignof ==\n");
    printf("_Alignof(char) = %zu\n", _Alignof(char));
    printf("_Alignof(int) = %zu\n", _Alignof(int));
    printf("_Alignof(struct unordered) = %zu\n", _Alignof(struct unordered));

    printf("\n== __attribute__((packed)) ==\n");
    printf("struct packed_example: sizeof = %zu\n", sizeof(struct packed_example));
    printf("  offsetof(a) = %zu\n", offsetof(struct packed_example, a));
    printf("  offsetof(b) = %zu\n", offsetof(struct packed_example, b));
    printf("  offsetof(c) = %zu\n", offsetof(struct packed_example, c));

    struct unordered u;
    struct reordered r;
    struct packed_example p;

    printf("\n== In-memory sizes ==\n");
    printf("sizeof(u) = %zu (with padding)\n", sizeof(u));
    printf("sizeof(r) = %zu (with less padding)\n", sizeof(r));
    printf("sizeof(p) = %zu (no padding)\n", sizeof(p));

    return 0;
}
```

**Predict: What is the difference in size between `struct unordered` and `struct reordered`? What is `offsetof(struct reordered, a)`?**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic struct_all.c -o struct_all && ./struct_all
```

Output (some warnings come from `_Alignof` not being standard in C99):

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
struct_all.c: In function ‘main’:
struct_all.c:37:38: warning: ISO C99 does not support ‘_Alignof’ [-Wpedantic]
   37 |     printf("_Alignof(char) = %zu\n", _Alignof(char));
      |                                      ^~~~~~~~
struct_all.c:38:37: warning: ISO C99 does not support ‘_Alignof’ [-Wpedantic]
   38 |     printf("_Alignof(int) = %zu\n", _Alignof(int));
      |                                     ^~~~~~~~
struct_all.c:39:50: warning: ISO C99 does not support ‘_Alignof’ [-Wpedantic]
   39 |     printf("_Alignof(struct unordered) = %zu\n", _Alignof(struct unordered));
      |                                                  ^~~~~~~~
== Padding and sizeof ==
struct unordered:  sizeof = 12
  offsetof(a) = 0
  offsetof(b) = 4
  offsetof(c) = 8

struct reordered:  sizeof = 8
  offsetof(b) = 0
  offsetof(a) = 4
  offsetof(c) = 5

== _Alignof ==
_Alignof(char) = 1
_Alignof(int) = 4
_Alignof(struct unordered) = 4

== __attribute__((packed)) ==
struct packed_example: sizeof = 6
  offsetof(a) = 0
  offsetof(b) = 1
  offsetof(c) = 5

== In-memory sizes ==
sizeof(u) = 12 (with padding)
sizeof(r) = 8 (with less padding)
sizeof(p) = 6 (no padding)
```

</details>

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. The order of the members changes `sizeof` (12 → 8 bytes).
2. `offsetof(struct reordered, a)` is 4, because `int b` occupies offsets 0-3.
3. With `__attribute__((packed))` the padding is gone and the size becomes 6 bytes.

</details>

Next, try the following.

- Access the members of `struct packed_example` to get a feel for alignment violations
- Look at the padding locations in a memory dump (such as `hexdump`) of `struct unordered`

## Common pitfalls

**`sizeof(struct)` is too large**
If a member has a large type, the alignment requirement of the whole struct goes up and the padding grows. Try arranging the order of the members.

**`offsetof` is different from what you expected**
The alignment requirement shifts the position of members. Check with `offsetof`.

**After using `__attribute__((packed))`, access to members is slow / crashes**
This is unaligned access. Limit `packed` to cases that are "safe as a byte sequence" (network protocols, I/O memory).

**`_Alignof` is not supported in C99 (warning)**
It is a compiler extension. With GCC/Clang, it is fine to remove `-Wpedantic` or use `-std=gnu99`.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c06_struct_align` — structs and alignment

```bash
./drill run c06
```

If you get stuck, use `./drill hint c06`. From the exercise side, `./drill read c06` brings you back to this chapter.

## References

- [cppreference: Alignment](https://en.cppreference.com/w/c/language/object)
- [Bit packing in C structs](https://en.cppreference.com/w/c/language/struct)

---

Previous chapter → [5. Strings](05_strings.md)
Next chapter → [7. Bit operations and register access](07_bit_operations_and_registers.md)
