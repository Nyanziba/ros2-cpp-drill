# c02: Fixed-width integers and implicit conversions

Use the fixed-width integer types of C (`uint8_t`, `int16_t`, and so on) to understand what happens on integer overflow.

## Learning goals

- Using the fixed-width integer types defined in `stdint.h`
- Overflow behavior of unsigned integers (automatic wrap)
- Overflow detection and saturating arithmetic for signed integers
- The basics of bit operations

## Functions to implement

1. **`uint8_t add_modulo_256(uint8_t a, uint8_t b)`**
   - Add two unsigned 8-bit integers
   - If the result exceeds 256, it wraps automatically

2. **`int16_t saturate_add(int16_t a, int16_t b)`**
   - Add two signed 16-bit integers
   - If it goes above INT16_MAX, stop at INT16_MAX
   - If it goes below INT16_MIN, stop at INT16_MIN

3. **`int check_high_bit(uint8_t value)`**
   - Check whether the most significant bit (0x80 = 128) is set
   - Return 1 if it is set, and 0 if it is not

## Tests

```bash
colcon test --packages-select drill_c02_fixed_width_int
```

## Hints

- The automatic wrap on `uint8_t` overflow is guaranteed by C99
- The range of `int` is wider than `int16_t`, so cast to `int` before checking in the calculation
- Use the `&` operator for bit operations
