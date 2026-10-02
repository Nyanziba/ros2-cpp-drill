# dp04 Factory Method [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 4. The base class fixes the steps of creation, and only **what to create** is left to the derived class.
The C++ theme is one thing: **pass the ownership of the created object to the caller with `std::unique_ptr`.**

## What to do

Fill in the 5 TODOs in `src/logger_factory.cpp`.

1. **`Logger::Logger` / `Logger::~Logger`**
   - If `alive_counter_` is not `nullptr`, increase and decrease it (to observe the number of live objects from outside)
2. **`MemoryLogger::write()`**
   - Add one line `"[tag] message"` to `lines_`
3. **`LoggerFactory::create()`** ← the template method
   - `create_logger()` → if `nullptr`, return `nullptr` without registering → `register_logger()` → return
4. **`MemoryLoggerFactory::create_logger()`** ← the factory method
   - `std::make_unique<MemoryLogger>(...)`. If the tag is an empty string, `nullptr`
5. **`MemoryLoggerFactory::register_logger()`**
   - Record the tag in `registered_tags_`

Do not edit `include/drill/logger_factory.hpp` and `test/test_exercise.cpp`.

## Run it

```bash
./drill run dp04
```

## Common pitfalls

- At the end of `create()`, `return logger;` is enough. **It is a local variable, so it is moved automatically.**
  If you write `return std::move(logger);`, you get in the way of the move optimization
- The return type of `create_logger()` is `std::unique_ptr<Logger>`.
  If you change it to `std::unique_ptr<MemoryLogger>`, **covariant return types do not work** and you get a compile error
  (returning `std::make_unique<MemoryLogger>(...)` as a `unique_ptr<Logger>` is fine)
- A creation failure is expressed with `nullptr`. **Do not throw exceptions** (they cannot be used on microcontrollers)
- `register_logger()` takes `const Logger &`. Ownership is not passed

## Tests

```bash
./drill run dp04
```

There are 6 tests. 3 of them check that **ownership moves to the caller**.

- Is the object destroyed when it leaves the scope (the factory does not own it)
- Does the owner change with `std::move`
- If the factory and the created object disappear at the same time, is there no double free

## References

- [4. Factory Method](../../docs-en/patterns/04_FactoryMethod.md)
- [C++ 6. Smart pointers](../../docs-en/cpp/06_smart_pointers.md)
- [cppreference: std::make_unique](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)
