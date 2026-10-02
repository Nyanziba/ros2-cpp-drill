# dp09 Bridge [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 9. You separate the "class hierarchy of functions" and the "class hierarchy of implementations".
Then you write **a Pimpl, which has the same structure but a different purpose**.

## What to do

### 1. `src/telemetry_view.cpp` — Bridge

The class hierarchy of implementations (where to output).

1. **`RecordingSink::open()` / `put_line()` / `close()`**
   - `open()` pushes `"<open>"` and `close()` pushes `"<close>"` to `log_`
   - `put_line()` pushes the received line as it is

2. **`NumberedSink::open()` / `put_line()` / `close()`**
   - `put_line()` adds a 0-based serial number, like `"0: v=12.4"`
   - `open()` resets the number to 0

The class hierarchy of functions (what to output).

3. **`TelemetryView::show()`**
   - Call `sink()` in the order `open()` → `put_line(text)` → `close()`

4. **`RepeatView::show_repeat()`**
   - `open()` once, `put_line(text)` `times_` times, `close()` once
   - Even if `times_` is 0 or less, call `open()` and `close()`

### 2. `src/link_stats.cpp` — Pimpl

5. **`struct LinkStats::Impl`** — give it a member that holds the samples
6. **`LinkStats::LinkStats()`** — create `impl_` with `std::make_unique<Impl>()`
7. **`add_sample()` / `count()` / `mean()` / `max()`** — if there are 0 samples, both `mean()` and `max()` are `0.0`

`~LinkStats()` and the 3 move functions are **already written. Do not delete them.**
If you delete them and write `= default` on the header side, it does not compile because of the incomplete type.
The reason is in section 9.4 of the article.

## Run it

```bash
./drill run dp09
```

## Common pitfalls

- `TelemetryView` does **not inherit** `TelemetrySink`. It just holds one as a member.
  If you use inheritance, M functions × N implementations need M×N classes
- If you write "where it goes" inside `show()`, the Bridge is broken.
  Do nothing other than calling `sink()`
- If you make the destructor of `LinkStats` `= default` in the header, you get
  `error: invalid application of 'sizeof' to an incomplete type 'LinkStats::Impl'`
- Before you write `count()` with `impl_->...`, create `impl_` in the constructor.
  If you forget, it is a nullptr reference

## Tests

```bash
./drill run dp09
```

There are 9 tests. They check that all 4 combinations of 2 kinds of functions × 2 kinds of implementations work,
that the function side is the same even if you swap the implementation,
and that the size of the Pimpl version is one pointer (`static_assert`).

## References

- [9. Bridge](../../docs-en/patterns/09_Bridge.md)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- [cppreference: PImpl](https://en.cppreference.com/w/cpp/language/pimpl)
