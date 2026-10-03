# c11 Endianness and serialization [C track]

Based on the CAN protocol specification, in this exercise you convert floating-point numbers and integers into a byte sequence in little-endian format, and restore them again.

## What to implement

Implement the following functions in `src/endian_serialize.c`. You **do not edit** the header (`include/drill/endian_serialize.h`).

| Function | Role |
| --- | --- |
| `write_float_le` | Split a `float` into 4 bytes in little-endian, and write them into `data[]` from the given offset |
| `read_float_le` | Restore the 4 bytes read in little-endian into a `float` |
| `write_uint32_le` | Split a `uint32_t` into 4 bytes in little-endian, and write them into `data[]` from the given offset |
| `read_uint32_le` | Restore the 4 bytes read in little-endian into a `uint32_t` |
| `build_speed_target_command` | Build the speed target command payload (8 bytes) |

## Implementation rules

**Especially important: when you convert a floating-point number to bytes, use `memcpy`, not a pointer cast.**

```c
// NG: strict aliasing violation (undefined behavior)
*(uint32_t*)&f;

// OK: the way defined by the standard
uint32_t u32;
memcpy(&u32, &f, sizeof(u32));
```

### Implementing `write_float_le`

1. Convert the `float` to a `uint32_t` with `memcpy`
2. Take out one byte at a time with shift operations and `& 0xFF`
3. Write into `data[]` in little-endian order (the least significant byte first)

### Implementing `read_float_le`

1. Cast each byte to `(uint8_t)`, then `widen` it (fill the upper bits with 0), and assemble a `uint32_t`
2. Convert the `uint32_t` to a `float` with `memcpy`
3. Return the `float`

**Important**: `char` is signed on many implementations, so when you read a byte value, always cast it to `(uint8_t)` first, and then widen that value to `uint32_t`.

### `build_speed_target_command`

The specification of the **speed target command** of the example CAN protocol (fictional):

- **Byte 0**: Port ID (1-8, use the received value as it is)
- **Byte 1-4**: Target speed (`float`, LE)
- **Byte 5-7**: Filled with 0

## Run it

```bash
./drill run c11
```

**The tests are red until you start.** Since the header is not editable, the tests are set up to fail.

## Common pitfalls

- A **pointer cast is a strict aliasing violation**. Use `memcpy`
- **When you read a byte value, cast it to `(uint8_t)`** and then widen it. This makes it work even with a signed `char`
- Check correctness with a **round-trip test**. Read back the value you wrote, and check that it returns to the original value
- The specification is based on fixed-width integer types such as `int8_t` and `int16_t`. Portability is required

## Tests

| Test | What it checks |
| --- | --- |
| `RoundTripsFloatOnePointZero` | Round-trip conversion of 1.0f (basic) |
| `RoundTripsFloatTwoPointFive` | Round-trip conversion of a fractional value |
| `RoundTripsNegativeFloat` | A negative floating-point number |
| `RoundTripsFloatZero` | Handling of zero |
| `RoundTripsFloatWithOffset` | Reading and writing with an offset |
| `RoundTripsUint32` | Round-trip conversion of a 32-bit integer |
| `RoundTripsUint32WithOffset` | Reading and writing with an offset |
| `RoundTripsUint32Zero` | uint32_t zero |
| `BuildsSpeedTargetCommand` | The basics of building the payload |
| `BuildsSpeedTargetCommandWithNegativeSpeed` | A negative speed value |
| `RoundTripsSpeedTargetCommand` | Building and restoring the payload |

## References

- [C track: 11. Endianness and serialization](../../docs-en/c/11_endianness_and_serialization.md)
