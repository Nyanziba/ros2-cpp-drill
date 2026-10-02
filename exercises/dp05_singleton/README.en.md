# dp05 Singleton [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 5. You make the **only UART the robot has** a singleton.

The reason is not "it is a hassle to pass the settings around".
The reason is that **there is physically only one**. If you cannot give this reason, do not make it a singleton
(section 0.2 of [0. Before you use them](../../docs-en/patterns/00_before_you_use_them.md)).

## What to do

Implement 5 places in `src/uart_port.cpp`. The key to all of them is a **function-local static** (Meyers Singleton).

1. **`UartPort::instance()`**
   - Put `static UartPort the_port;` inside the function and return a reference to it
   - The current implementation does `new` on every call, so you get a different object every time

2. **`UartPort::construction_count()`**
   - Return `g_uart_construction_count`

3. **`UartPort::reset()`**
   - Set `baud_rate_` to `kDefaultBaudRate` and empty `sent_lines_`
   - **Do not recreate the object. Only restore the state**

4. **`LazyProbe::instance()`**
   - The same: a function-local static

5. **`LazyProbe::was_constructed()`**
   - Return `g_lazy_probe_constructed`

Do not edit `include/drill/uart_port.hpp` and `test/test_exercise.cpp`.

## Run it

```bash
./drill run dp05
```

## Common pitfalls

- **A function-local static is thread-safe since C++11.** You do not need to write
  your own `std::mutex` (magic statics). If you do, it is even slower
- `instance()` returns a **reference**. If it returned a pointer, the caller
  could not tell whether it may `delete` it
- In `reset()` you may want to write `*this = UartPort{};`, but it **does not compile**.
  Both the copy assignment and the move assignment are `= delete`d. Restore the members one by one
- The 4 lines of `= delete` for copy and move are not decoration.
  If you remove them, `UartPort port = UartPort::instance();` compiles, and the singleton breaks.
  The `static_assert` at the top of the test watches for that
- Do not touch hardware inside the constructor.
  The reason is that **you do not know when it runs** (see "The conclusion for microcontrollers" in the article for details)

## Tests

```bash
./drill run dp05
```

There are 8 tests.

| Test | What it checks |
| --- | --- |
| `InstanceReturnsSameObjectEveryTime` | whether the addresses are the same |
| `StateIsSharedByTheOnlyInstance` | whether the same state is visible from a different path |
| `InitializationRunsOnlyOnceNoMatterHowManyInstanceCalls` | whether the constructor runs only once |
| `ResetRestoresDefaultBaudRate` / `ResetClearsTransmitHistory` | the contents of `reset()` |
| `ResetDoesNotRecreateObject` | whether the address and the construction count do not change |
| `StateFromPreviousTestDoesNotLeak` | state leaking between tests |
| `InitializationWaitsForFirstInstanceCall` | whether it is lazy initialization |

It also checks with `static_assert` that copy, move, and construction from outside are prohibited.

## References

- [5. Singleton](../../docs-en/patterns/05_Singleton.md)
- [C++ Basics 7. static](../../docs-en/cpp-basics/07_static.md)
- [cppreference: Storage class specifiers (static local variables)](https://en.cppreference.com/w/cpp/language/storage_duration)
