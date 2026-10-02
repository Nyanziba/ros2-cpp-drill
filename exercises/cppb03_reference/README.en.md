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
| `SwapsTwoVariablesByReference` | changes through references |
| `ReturnsReferenceSoCallerCanModify` | reference return values |
| `ReturnsFirstWhenEqual` | edge case |

## References

- [3. References](../../docs-en/cpp-basics/03_references.md)
