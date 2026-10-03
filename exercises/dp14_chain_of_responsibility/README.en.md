# dp14 Chain of Responsibility [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 14. You write the robot's fault detection (voltage drop, overcurrent, communication loss)
as a chain that passes the fault to different handlers step by step.

The theme of this chapter is not the structure but the **lifetime of the chain**.
You implement **both** the version where `next` is owned by `std::unique_ptr` (the GoF version)
and the version that builds no chain and goes through an array in order, and compare them.

## What to do

Implement 6 things in `src/fault_chain.cpp`.

1. **`FaultHandler::~FaultHandler()`**
   - If `destruction_log_` is not `nullptr`, `push_back` `name_`
   - This is a mechanism so that the test can observe that "when the head was dropped, the whole chain disappeared"

2. **`FaultHandler::set_next()`**
   - Move `next` into `next_` and own it
   - **The return value is `*next_`, not `*this`**. This is so that `a.set_next(b).set_next(c)` works

3. **`FaultHandler::support()`**
   - Its own `resolve()` → if that fails, `next_->support()` → if there is no next one either, `std::nullopt`
   - **Do not throw exceptions**

4. **`FaultHandler::support_alone()`**
   - Do not pass it on; return only the result of its own `resolve()`

5. **`dispatch()`**
   - The method that builds no chain. Ask the array from the head with `support_alone()`

6. **`resolve()` of `LowVoltageHandler` / `OverCurrentHandler` / `CommTimeoutHandler`**
   - If it can handle it, a `FaultAction`; if not, `std::nullopt`
   - **Do not write "pass it on" here** (NVI)

| Handler | Condition it handles | action |
| --- | --- | --- |
| `LowVoltageHandler` | `kLowVoltage` and `magnitude <  threshold_mv` | `"reduce_duty"` |
| `OverCurrentHandler` | `kOverCurrent` and `magnitude >= limit_ma` | `"cut_output"` |
| `CommTimeoutHandler` | `kCommTimeout` and `magnitude >= timeout_ms` | `"safe_stop"` |

**Nobody handles** `kEncoderSlip`. You check that it becomes `std::nullopt`.

## Run it

```bash
./drill run dp14
```

## Common pitfalls

- **After** you `std::move(next)` in `set_next()`, `next` is empty.
  If you return `*next`, it crashes. Put it into `next_` and then return `*next_`
- If `set_next()` returns `*this`, then in `a.set_next(b).set_next(c)`
  **b is silently freed**. The test `SetNextReturnsReferenceToNextHandler` fails it
- After the destructor body runs, the member `next_` is destroyed.
  That is, the destruction log is in the order **from the head to the tail**
- If you want to touch `next_` inside `resolve()`, the design is broken.
  Passing it on is the job of `support()` only
- "Nobody handled it" is `std::nullopt`. Do not `throw`
  (on microcontrollers `-fno-exceptions` is normal)

## Tests

```bash
./drill run dp14
```

There are 10 tests. They check that the handler that handles the fault changes when you swap the order,
and that when you destroy the head, the whole chain is destroyed (the order of the destructor log).

## References

- [14. Chain of Responsibility](../../docs-en/patterns/14_ChainOfResponsibility.md)
- [cppreference: std::optional](https://en.cppreference.com/w/cpp/utility/optional)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
