# c02 Initialize a class [C++]

Learn class constructors and const members.

## What to do

Complete the constructor in `src/stopwatch.cpp`.

Use the member initializer list to initialize `max_time_` and `elapsed_`:

```cpp
Stopwatch::Stopwatch(int max_time_ms)
  : max_time_(??), elapsed_(??)
{
}
```

Also implement the member functions `advance()`, `elapsed()`, and `max_time()`.

## Run it

```bash
./drill run c02
```

## Common pitfalls

- A **const member variable** cannot be initialized by a normal assignment inside the constructor.
  You must initialize it with the **member initializer list**.
- The member initializer list has the form `: variable(value), ...`. Separate the items with commas, not semicolons.
- This is how you write the `const` keyword on a const member function: `int elapsed() const { ... }`
  If you change a member variable inside it, you get a compile error.

## Tests

```bash
./drill run c02
```

| Test | What it checks |
| --- | --- |
| `ConstructorSetsMaxTime` | correct use of the member initializer list |
| `ElapsedTimeIsZeroInitially` | initialization of elapsed_ |
| `AdvanceIncreasesElapsedTime` | the implementation of the advance() method |
| `ConstMemberFunctionsReturnValues` | the syntax of const member functions |

## References

- [cppreference: Initializer list](https://en.cppreference.com/w/cpp/language/initializer_list)
- [cppreference: const (qualifier)](https://en.cppreference.com/w/cpp/language/const)
- [2. Initialize a class](../../docs-en/cpp/02_classes_and_initialization.md)
