# c06 Express ownership with smart pointers [C++]

Learn weak_ptr and the Registry pattern.

## What to do

Implement the Registry class in `src/registry.cpp`.

Registry is a class that manages references to Items.
It uses weak_ptr, and fire() processes only the Items that are still held outside.

Functions to implement:
- `add()`: convert an Item to a weak_ptr and register it
- `fire()`: iterate over the registered Items and print only the ones that are alive

## Run it

```bash
./drill run c06
```

## Common pitfalls

- `std::weak_ptr` does not increase the reference count (of shared_ptr).
- You can check with `expired()` whether the object is alive.
- `lock()` converts a weak_ptr to a shared_ptr.

## Tests

```bash
./drill run c06
```

| Test | What it checks |
| --- | --- |
| `生きているItemだけが出力される` (only alive Items are printed) | the expired() check of weak_ptr |

## References

- [cppreference: weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [cppreference: shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr)
- [6. Express ownership with smart pointers](../../docs-en/cpp/06_smart_pointers.md)
