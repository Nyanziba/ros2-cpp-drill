# c07 Register lambdas and callbacks [C++]

Learn lambda expressions and std::function.

## What to do

Implement the CallbackManager class in `src/callbackmanager.cpp`.

CallbackManager is a class that registers and runs callback functions.

Functions to implement:
- `register_callback()`: add a std::function to the list
- `fire()`: call all the registered callbacks

## Run it

```bash
./drill run c07
```

## Common pitfalls

- `std::function<void(int)>` can hold any void(int) callback.
- A lambda expression `[&x](int v) { ... }` can capture values and references.

## Tests

```bash
./drill run c07
```

| Test | What it checks |
| --- | --- |
| `コールバックが呼び出される` (the callback is called) | the behavior of register_callback and fire |
| `複数のコールバックが登録できる` (you can register multiple callbacks) | managing multiple callbacks |

## References

- [cppreference: Lambda expressions](https://en.cppreference.com/w/cpp/language/lambda)
- [cppreference: std::function](https://en.cppreference.com/w/cpp/utility/functional/function)
- [7. Lambdas and callbacks](../../docs-en/cpp/07_lambdas_and_std_bind.md)
