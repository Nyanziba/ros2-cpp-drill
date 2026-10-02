# dp12 Decorator [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 12. You add log formatting by **wrapping** a "plain message".
`SinkDecorator` / `LogSink` correspond to Border/Display.

The C++ theme is one thing: **ownership**. A decorator owns its inner object with `std::unique_ptr<LogSink>`.
The constructor takes it by value and puts it into the member with `std::move`.

## What to do

Implement it in `src/log_sink.cpp`.

1. **`join_tag()`**
   - Join the tag and the body with one half-width space
   - It is called from **both** the `unique_ptr` version and the template version. This is why the two match

2. **`PlainMessage`**
   - `format()` returns the message as it is, adding nothing
   - In the destructor, `DestructionLog::record("PlainMessage")`

3. **`SinkDecorator`**
   - Constructor: move the received `inner` into the member with `std::move`
   - `inner()`: return a **reference** to the inner object it owns

4. **`LevelTag` / `TimestampTag` / `SourceTag`**
   - `format()` "lets the inner object format first, and then prepends the tag to that result"
   - In each destructor, record its own name

5. **`plain()` / `with_level()` / `with_timestamp()` / `with_source()`**
   - Assembly helpers. They take the place of writing `std::make_unique` three levels deep

## Run it

```bash
./drill run dp12
```

## Common pitfalls

- `SinkDecorator(std::unique_ptr<LogSink> inner) : inner_(inner)` is a compile error.
  A `unique_ptr` cannot be copied. Write **`std::move(inner)`**
- `inner()` returns `const LogSink &`. `LogSink` is an abstract class, so it cannot be returned by value
- In `format()`, **call the inner object first**. If you get the order wrong, the position of the tag is swapped
- Decorator means the output changes with the order of wrapping. The tests check that
- If you forget to record in the destructor, `DestroyingOutermostDestroysAllInner` fails

## Tests

There are 10 tests. They check that the output changes with the order of wrapping, that you can wrap many layers,
that dropping the one outer object frees all the inner ones,
and that the template version gives the same output and `std::is_polymorphic_v` is `false`.

## References

- [12. Decorator](../../docs-en/patterns/12_Decorator.md)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- [cppreference: std::move](https://en.cppreference.com/w/cpp/utility/move)
