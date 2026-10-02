# cppb05 Arrays and pointers [C++ Basics]

Learn array decay and how to handle the element count.

## What to do

Implement sum() and find_first() in `src/array_util.cpp`.

- `sum()`: return the sum of all elements of the array
- `find_first()`: return a pointer to the first element equal to target (nullptr if not found)

## Run it

```bash
./drill run cppb05
```

## Common pitfalls

- An array decays to a pointer when passed as a function argument. You must pass the element count separately.
- `std::size_t` is an unsigned type that depends on the platform. Be careful when you compare it with a signed int.

## Tests

```bash
./drill run cppb05
```

| Test | What it checks |
| --- | --- |
| `Sum` | looping over an array |
| `Sum空の配列` (Sum of an empty array) | handling count==0 |
| `FindFirst見つかる` (FindFirst finds it) | returning a pointer |
| `FindFirst見つからない` (FindFirst does not find it) | returning nullptr |

## References

- [5. Pointers (2)](../../docs-en/cpp-basics/05_pointers_2.md)
