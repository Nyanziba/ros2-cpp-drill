# cppb10 Split into headers [C++ Basics]

In this exercise you separate declarations and definitions and link several .cpp files.

## What to do

Implement add() in `src/math.cpp` and multiply() in `src/util.cpp`.

- `include/drill/math.hpp`: declarations only (do not edit)
- `src/math.cpp`: the definition of add()
- `src/util.cpp`: the definition of multiply()
- Test: uses both

## Run it

```bash
./drill run cppb10
```

## Common pitfalls

- It is OK for several `.cpp` files to `#include` the same header (they are only declarations).
- At build time, the two object files are linked into one executable.
- Do not forget the include guard (`#pragma once`).

## Tests

```bash
./drill run cppb10
```

| Test | What it checks |
| --- | --- |
| `AddsIntegers` | math.cpp |
| `MultipliesIntegers` | util.cpp |
| `UsesAddAndMultiplyTogether` | the link succeeds |

## References

- [10. Headers and project layout](../../docs-en/cpp-basics/10_headers_and_project_layout.md)
