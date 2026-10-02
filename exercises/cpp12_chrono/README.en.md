# c10 Handle time with types [C++]

Learn std::chrono.

## What to do

Implement two functions in `src/timer.cpp`:

1. **count_ticks()**
   - Calculate the number of ticks from a budget in milliseconds
   - Use `duration_cast`

2. **seconds_to_ms()**
   - Convert seconds (double) to std::chrono::milliseconds

## Run it

```bash
./drill run c10
```

## Common pitfalls

- Use `std::chrono::duration_cast<T>(d)` to convert the type
- Use `duration.count()` to get the number
- Be careful about the precision of time calculations

## Tests

```bash
./drill run c10
```

## References

- [cppreference: std::chrono](https://en.cppreference.com/w/cpp/chrono)
- [cppreference: duration_cast](https://en.cppreference.com/w/cpp/chrono/duration/duration_cast)
- [10. Handle time with types](../../docs-en/cpp/12_chrono_and_time.md)
