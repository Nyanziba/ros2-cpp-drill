# c01 Get the build and link to pass [C++]

**In this exercise only, the build fails right away. That is the first problem.**

A C++ build has two stages: compile and link.
This exercise has one job in each stage.

## What to do

You edit **two files**.

| File | What to fix |
| --- | --- |
| `include/drill/counter.hpp` | Remove the link error (`multiple definition`) |
| `src/counter.cpp` | Implement `next_id()` |

### TODO(1) — Remove the link error

First, run `./drill run c01`. Not a single test runs, and it fails like this.

```
multiple definition of `add_one(int)'
```

**`add_one` is written only once in the source, but you are told "there are multiple definitions".**
Think about why this happens, and then read the comment in the header.

There are two ways to fix it, and both pass.

- Add `inline` to the definition of `add_one`
- Move the definition to `src/counter.cpp` and leave only the declaration in the header

### TODO(2) — Implement `next_id()`

| Item | Value |
| --- | --- |
| Behavior | Returns a value that grows 1, 2, 3, ... on each call |
| Return type | `int` |
| First call | `1` |
| Second call onward | One more than the previous value |

Hint: use a **`static` local variable**.

## Run it

```bash
./drill run c01
```

Until you fix TODO(1), not a single test runs.
**If the link does not pass, no executable is created, so there is nothing to test.**
This exercise is meant to let you feel the difference between a "compile error" and a "link error".

You can also look at the symbols directly.

```bash
nm -C build/drill_cpp01_build_and_link/CMakeFiles/drill_cpp01_build_and_link_lib.dir/src/counter.cpp.o | grep add_one
```

Before you add `inline` it shows `T add_one(int)`, and after you add it it shows `W add_one(int)`.
`T` means "this is the only definition", and `W` (weak symbol) means "other identical definitions are allowed".

## Common pitfalls

- **It is duplicated even though there is `#pragma once`.** `#pragma once` only guarantees
  that the header is not pasted twice into the same translation unit. If you have two `.cpp` files,
  you have two translation units.
- **`inline` is not "to make it faster".** The optimizer decides by itself whether to expand the call.
  In modern C++, the real meaning of `inline` is this relaxation: "multiple definitions are allowed".
- **A `static` local variable is initialized only the first time.** From the second call on,
  the previous value is still there.
- If you put `static` outside a function, it means something else: "a variable visible only from this file".
  A narrower scope is safer, so put it inside the function.
- Return `++id` (increment, then return). If you use `id++`, the first call returns 0.

## Tests

```bash
./drill run c01
```

| Test | What it checks |
| --- | --- |
| `add_oneが1増やす` (add_one adds 1) | whether you fixed TODO(1) and the build passes |
| `next_idが順番に増える` (next_id increases in order) | whether the `static` local variable keeps its value |

## References

- [1. How build and link work](../../docs-en/cpp/01_how_build_and_link_work.md) — Section 1.4 covers the ODR, and section 1.5 covers how to read link errors
- [cppreference: Storage duration](https://en.cppreference.com/w/cpp/language/storage_duration)
- [cppreference: inline specifier](https://en.cppreference.com/w/cpp/language/inline)
