# cppb03 Receive by reference [C++ Basics]

Learn to change values through references.

## What to do

Implement swap_values() and largest() in `src/swapper.cpp`.

- `swap_values()`: swap the caller's two ints
- `largest()`: return a reference to the larger one

## Run it

```bash
./drill run cppb03
```

## Common pitfalls

- A reference `&` is an alias. If you change the reference, the caller's value changes too.
- When you return a reference, make sure the object does not leave its scope.

## Tests

```bash
./drill run cppb03
```

| Test | What it checks |
| --- | --- |
| `参照経由のSwap` (swap through references) | changes through references |
| `参照を返して呼び出し元を変更` (return a reference and change the caller's value) | reference return values |
| `等しい場合は最初の方` (if equal, return the first one) | edge case |

## References

- [3. References](../../docs-en/cpp-basics/03_references.md)
