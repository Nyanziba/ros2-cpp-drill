# dp20 Flyweight [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 20. You share the calibration table of each model number among dozens of sensors.

Flyweight is **the only pattern that exists for optimization**. In practice, the subject of this exercise would
end with a `constexpr` table (`kCalibrationRom` in `include/drill/calibration.hpp`).
We still write it by hand once so that **you can judge when "constexpr is the end of it"**.

## What to do

Implement 3 things in `src/calibration.cpp`.

1. **`CalibrationRegistry::get()`**
   - If there is a live one in the pool, return it (`std::weak_ptr::lock()`)
   - If `lock()` is `nullptr`, it is "a leftover nobody uses any more". Create it again
   - If `find_spec(model_id)` is `nullptr` (a model number not in the ROM), return `nullptr`. Do not throw an exception
   - Create it with `std::make_shared` and register it in the pool **as a `weak_ptr`**

2. **`CalibrationRegistry::sweep_expired()`**
   - Remove the entries that are `expired()` and return the number removed

3. **`Sensor::convert()`**
   - `raw * gain + offset + zero_offset`
   - `gain` / `offset` are shared (intrinsic state), and `zero_offset` is per individual (extrinsic state)
   - If the table is `nullptr`, `0.0`

The `constexpr` version (`kCalibrationRom` / `find_spec`) is **already implemented**. Read it first.

## Run it

```bash
./drill run dp20
```

## Common pitfalls

- **Do not put a `shared_ptr` in the pool.** If you do, the pool keeps holding a reference and
  nothing is freed until the process ends. The test looks at `use_count`.
  With 2 users, `use_count` is **2**. If it is 3, the pool is counting
- **Even with `weak_ptr`, the entry of the `map` is not removed automatically.**
  This is because a `weak_ptr` cannot tell the `map` that it has expired.
  That is why you need `sweep_expired()`
- In `sweep_expired()`, write `it = pool_.erase(it)`.
  If you `++it` and then `erase`, you touch an invalid iterator
- Do not put `zero_offset` on the `CalibrationTable` side.
  The moment you do, sensors of the same model number can no longer share it
- `Handle` is `std::shared_ptr<const CalibrationTable>`. It has `const`.
  This is because if one person rewrites something that is shared, it affects everyone

## Tests

```bash
./drill run dp20
```

There are 7 tests.

- If you get the same model number twice, **the same address** is returned
- The number of creations matches **the number of kinds** (3), not the number of lookups (5)
- When everyone lets go, it is destroyed, and it also disappears from the pool with `sweep_expired()`
- Extrinsic state is not shared
- The `constexpr` table passes `static_assert`, and there are **0 heap allocations at run time**
  (the test replaces the global `operator new` and counts)

## References

- [20. Flyweight](../../docs-en/patterns/20_Flyweight.md)
- [cppreference: std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [cppreference: std::string_view](https://en.cppreference.com/w/cpp/string/basic_string_view)
