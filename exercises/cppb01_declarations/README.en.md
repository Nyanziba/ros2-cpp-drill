# cppb01 Reading declarations [C++ Basics]

Understand how `const` and pointers combine.

## What to do

Write the declarations and definitions in `include/drill/reader.hpp` and `src/reader.cpp`.

Functions:
- `int read_value(const int * p)`: read a value from a pointer to const int
- `void modify_value(int * p, int new_val)`: change a value through a pointer to int
- `const int * get_constant_ptr(const int * p)`: return a pointer to const int

## Run it

```bash
./drill run cppb01
```

## Common pitfalls

- Read a declaration from right to left. `const int *` means "a pointer to const int".
- To read or write what a pointer points to, use the dereference `*p`.

## Tests

```bash
./drill run cppb01
```

| Test | What it checks |
| --- | --- |
| `ReadsThroughConstPointer` | reading through `const int *` |
| `ModifiesThroughNonConstPointer` | writing through `int *` |
| `ReturnsConstPointerToOriginal` | the return value `const int *` |

## References

- [1. Reading declarations](../../docs-en/cpp-basics/01_reading_declarations.md)
