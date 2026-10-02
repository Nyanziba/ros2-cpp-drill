# c09 Use the standard library [C++]

Learn std::clamp and std::optional.

## What to do

Implement two functions in `src/limiter.cpp`:

1. **clamp_velocity()**
   - Use `std::clamp` to limit a value to a range

2. **find_user_id()**
   - Look up a value in a `std::map`
   - Return the result with `std::optional`

## Run it

```bash
./drill run c09
```

## Common pitfalls

- Use `std::clamp(val, min, max)` to limit a value to the range min to max
- `std::optional<T>` represents both the case where there is a value and the case where there is none
- `std::nullopt` means "no value"

## Tests

```bash
./drill run c09
```

## References

- [cppreference: std::clamp](https://en.cppreference.com/w/cpp/algorithm/clamp)
- [cppreference: std::optional](https://en.cppreference.com/w/cpp/utility/optional)
- [9. The standard library](../../docs-en/cpp/11_standard_library_toolbox.md)
