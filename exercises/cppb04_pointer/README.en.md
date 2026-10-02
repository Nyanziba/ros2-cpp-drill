# cppb04 Use pointers [C++ Basics]

Learn the nullptr check and pointer dereference.

## What to do

Implement try_read() in `src/checker.cpp`.

- If neither pointer is nullptr, write the value that the read pointer points to into the place that the write pointer points to
- If either one is nullptr, return false

## Run it

```bash
./drill run cppb04
```

## Common pitfalls

- A `nullptr` check prevents segmentation faults.
- Dereferencing `*p` assumes that `p` is not nullptr.

## Tests

```bash
./drill run cppb04
```

| Test | What it checks |
| --- | --- |
| `両方がvalidな場合` (both are valid) | normal case |
| `読み込みポインタがnullptr` (the read pointer is nullptr) | nullptr check |
| `書き込みポインタがnullptr` (the write pointer is nullptr) | nullptr check |
| `両方がnullptr` (both are nullptr) | check of both |

## References

- [4. Pointers (1)](../../docs-en/cpp-basics/04_pointers_1.md)
