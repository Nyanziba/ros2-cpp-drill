# dp18 Memento [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 18. Using the tuning history of PID gains as the subject, you implement Memento in 2 ways.
**An ordinary version that uses value semantics**, and **a fixed-length ring buffer version for microcontrollers**.

## What to do

Implement 7 things in `src/gain_tuner.cpp`.

1. **`GainTuner::create_snapshot()`**
   - Return a `GainSnapshot` that holds the current `kp_` / `ki_` / `kd_` / `label_` **by value**
   - The constructor of `GainSnapshot` is private, but `GainTuner` is a `friend`, so it can call it

2. **`GainTuner::restore(const GainSnapshot &)`**
   - Write the contents of the Memento back to itself. **Copy** `label_`
   - The snapshot side does not change. You can restore it again later

3. **`GainTuner::restore(GainSnapshot &&)`**
   - The move version. Take `label_` with `std::move`
   - After taking it, call `snapshot.label_.clear()`.
     The contents of a moved-from `std::string` are "unspecified" by the standard,
     so if you promise "it becomes empty", empty it yourself

4. **`GainTuner::capture_state()` / `restore_state()`**
   - The POD path for microcontrollers. `GainState` has only `kp` / `ki` / `kd`
   - `restore_state()` does not touch `label_`

5. **`GainHistory::push()` / `size()` / `recent()`**
   - A fixed-length ring buffer. Do not use `std::vector`
   - If it is full, drop the oldest one
   - `recent(0)` is the newest, and `recent(1)` is the one before it

## Run it

```bash
./drill run dp18
```

## Common pitfalls

- **Hold the members of the Memento by value.** If you hold them with `const State &`, `State *`, or
  `std::shared_ptr<State>`, it is not a snapshot.
  If you change the original, the Memento changes with it.
  What `shared_ptr` prevents is a **lifetime** problem, not sharing
- It is intended that the constructor of `GainSnapshot` is private.
  C++ has no mechanism that corresponds to package private in Java, so we express it with `friend`.
  If you touch `saved.kp_` from outside, you get
  `error: 'kp_' is a private member of 'GainSnapshot'`
- **Do not prohibit** the copy of `GainSnapshot`.
  The Undo history is stacked in a `std::vector<GainSnapshot>`, so if you delete both copy and move,
  even `push_back` does not compile
- `head_` is "**the next position to write**". The newest is one before `head_`.
  So that the subtraction does not make `std::size_t` negative, add `kCapacity` and then take `%`
- If you want to add a `std::string` to `GainState`, stop.
  It can no longer be carried with `memcpy`, and the `static_assert` in the header stops the compile

## Tests

```bash
./drill run dp18
```

There are 9 tests. Besides that you can restore, they check
**that the Memento does not change even if you change the original after saving** (whether it is a snapshot),
that `GainSnapshot` cannot be constructed from outside,
and that when the ring buffer exceeds its capacity, it drops the oldest ones first.

## References

- [18. Memento](../../docs-en/patterns/18_Memento.md)
- [cppreference: std::is_trivially_copyable](https://en.cppreference.com/w/cpp/types/is_trivially_copyable)
- [cppreference: friend declaration](https://en.cppreference.com/w/cpp/language/friend)
