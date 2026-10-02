# dp15 Facade [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 15. You gather the robot's start-up sequence

```
power on → sensor init → calibration → link up
```

into **one entry point**. But in C++ there are 2 ways to make the entry point, so
you **implement both and check that they give the same result**.

1. **Namespace + free function version** `robot::start_once()` — it corresponds to `PageMaker` in Yuki's book.
   In C++ we do not write a class with only `static` methods
2. **RAII class version** `robot::RobotSession` — it has state, so it is a class.
   Initialize in the constructor and clean up in the destructor. **This is the main choice for the C++ Facade**

## What to do

Implement it in `src/robot_startup.cpp`. Do not edit the header.

### Part 1: The subsystems (8 free functions in an anonymous namespace)

1. **`power_on` / `sensor_init` / `calibrate` / `link_up`** —
   on success, add `"power_on"` and so on to the log and return `true`;
   on failure, add `"power_on_failed"` and so on and return `false`
2. **`power_off` / `sensor_deinit` / `calibration_clear` / `link_down`** — the clean-up side

**These 8 do not appear in the header.** If you put them in an anonymous namespace, they get internal linkage,
and from other translation units even the names do not exist.
"Reducing the surface you show" is the essence of Facade.

### Part 2: Gather the clean-up in one place

3. **`teardown(completed_stages, log)`** — receives the number of completed stages and undoes them in **reverse order**

```cpp
if (completed_stages >= 4) { link_down(log); }
if (completed_stages >= 3) { calibration_clear(log); }
if (completed_stages >= 2) { sensor_deinit(log); }
if (completed_stages >= 1) { power_off(log); }
```

The point is not fall-through but **independent `if`s in descending order**.

### Part 3: Namespace + free function version

4. **`robot::start_once()`** — call the 4 stages in order,
   and if one fails, stop there and `teardown(the number of successful stages, log)`.
   Even on success, call `teardown(4, log)` at the end (because it is an entry point that "starts once and stops right there")

There are 5 kinds of `return`, and you must **call `teardown()` at every one of them**.
If you forget one, nobody tells you. This is the motivation for the next RAII version.

### Part 4: RAII class version

5. **Constructor** — run the start-up sequence.
   Every time a stage succeeds, increase `completed_stages_`; when one fails, put it in `failed_stage_` and stop.
   **Do not write the clean-up here**
6. **Destructor** — just call `teardown(completed_stages_, log_)`
7. **Move constructor** — make the moved-from object empty
   (`completed_stages_ = 0` / `ready_ = false` / `log_ = nullptr`).
   **If you forget, the clean-up runs twice**
8. **`drive(duty)`** — if it is not `is_ready()`, do nothing and return `false`.
   Otherwise add to the log in the form `"drive:50"` and return `true`

## Run it

```bash
./drill run dp15
```

## Common pitfalls

- Even if you `return` in the middle of the constructor, **the destructor always runs**.
  This is because the object is already constructed. So you can write the rollback in the destructor
- After `std::move`, **the destructor runs** for the object too.
  It is not "it disappeared because it was moved". If you do not empty the moved-from object, the power is turned off twice
- If the power on fails, the number of successful stages is 0. **Do not call `power_off`**.
  The test `PowerOnFailureRunsNoCleanup` checks that
- `append()` takes `const char *`. `drive()` makes a `std::string`, so
  use `log_->push_back()` directly (`log_` can be `nullptr`)
- You may want to implement the free function version as "just create a `RobotSession` and throw it away", but
  **write it in the form that rolls back by hand.** Learning the difference from the RAII version by experience is the main point of this exercise

## Tests

```bash
./drill run dp15
```

There are 11 tests. They check that the steps run in the correct order,
that nothing after the failed stage runs, that only the stages that succeeded are rolled back in reverse order,
that the logs of the 2 versions match, that the clean-up does not run twice after a move,
and, with `static_assert`, that copy is prohibited and the constructor is `explicit`.

## References

- [15. Facade](../../docs-en/patterns/15_Facade.md)
- [cppreference: std::fstream](https://en.cppreference.com/w/cpp/io/basic_fstream)
- [cppreference: unnamed namespaces](https://en.cppreference.com/w/cpp/language/namespace)
