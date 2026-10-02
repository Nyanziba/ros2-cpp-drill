# dp10 Strategy [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 10. You implement **the same algorithm in 3 ways in C++**,
and check that the results match and that the costs differ.

The subject is a velocity command filter. You compare the raw command `raw` coming from the upper layer
with the previously output value `previous`, round it, and then pass it to the motor.

| Name | Calculation |
| --- | --- |
| clamp | Fit `raw` into `[-max_abs, +max_abs]` (`previous` is not used) |
| slew rate | Fit `raw` into `[previous - max_delta, previous + max_delta]` |

**The previous value is held not by the Strategy but by the Context (Commander).**
If you keep the Strategy stateless, it can be `const`, and you can share a `static` instance.
This is a decision that pays off on microcontrollers.

## What to do

Implement `src/velocity_filter.cpp`. It is divided into 3 blocks.

### 1) Virtual function version

- `ClampFilter::apply` / `SlewRateFilter::apply`
  - **Do not write a name** for an unused argument (you get `-Wunused-parameter`)
  - `std::clamp` is in `<algorithm>`
- The constructor / `set_filter` / `filter` / `update` of `VirtualCommander`
  - **Do not copy** the Strategy; **point to it by address**. Do not own it
  - If you copy it, it slices, and the derived `apply()` disappears

### 2) `std::function` version

- The constructor / `set_filter` / `update` of `FunctionCommander`
  - Take them with `std::move`
- `make_clamp_fn` — returns a lambda that does the same calculation as `ClampFilter::apply`
  - It captures `max_abs`. Because it has a capture, it cannot be converted to a function pointer

### 3) Template version

- `ClampPolicy::apply` / `SlewRatePolicy::apply`
  - The contents are not different by one character from the virtual function version. Your job is **not to write `virtual`**
  - `StaticCommander` is already written in the header (it is a template, so the definition must be in the header)

## Run it

```bash
./drill run dp10
```

## Common pitfalls

- **All three must give the same output.** Only the means differ; the algorithm is the same
- If `VirtualCommander` holds a copy of the Strategy, the test
  "仮想関数版はStrategyを所有せず参照で指している" (the virtual function version does not own the Strategy and points to it by reference) fails it by comparing addresses
- You may want to make `filter_` a `const VelocityFilter &` (a reference), but
  **a reference cannot be reseated**, so you cannot swap it. That is why it is a pointer
- `VirtualCommander` does **not own** the Strategy.
  If you destroy the `ClampFilter` object before the Commander, it is undefined behavior
- If you put `virtual` on `ClampPolicy`, it fails with `static_assert`.
  A vtable is added and `sizeof` also grows

## Tests

```bash
./drill run dp10
```

There are 9 tests. They check that the 3 ways match, swapping at run time,
that the Strategy is not owned (comparing addresses),
and that the template version is not polymorphic (`std::is_polymorphic_v` and `sizeof`).

## References

- [10. Strategy](../../docs-en/patterns/10_Strategy.md)
- [0. Before you use them](../../docs-en/patterns/00_before_you_use_them.md) — if there is only one implementation, do not introduce a Strategy
- [cppreference: std::function](https://en.cppreference.com/w/cpp/utility/functional/function)
- [cppreference: std::clamp](https://en.cppreference.com/w/cpp/algorithm/clamp)
- [cppreference: std::is_polymorphic](https://en.cppreference.com/w/cpp/types/is_polymorphic)
