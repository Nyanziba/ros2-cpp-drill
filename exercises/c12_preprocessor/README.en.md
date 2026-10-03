# c12 The preprocessor and debugging [C track]

This exercise teaches how to use the C preprocessor and the `assert` macro, and the C11 `_Static_assert`.

## The header already has two macros and one struct

You **do not edit** `include/drill/macro_utils.h`. The following are already defined.

| Element | Description |
| --- | --- |
| `SQUARE(x)` | A squaring macro (`(x) * (x)`). **Learn the importance of parentheses** |
| `DOUBLE_SQ(x)` | A multi-statement macro that squares a variable. **The `do { ... } while (0)` pattern** |
| `CanPayload` | An 8-byte CAN payload struct |
| `_Static_assert` | Checks `sizeof(CanPayload) == 8` at compile time |

## What to do

The only file you edit is `src/macro_utils.c`. Implement the following two functions.

### `is_debug_mode()`

Define the `DEBUG` macro in this file, and put it in debug mode (return 1).

```c
#define DEBUG 1

int is_debug_mode(void)
{
  return DEBUG;
}
```

### `validate_port_id()`

Check that the port ID (the received `uint8_t`) is **in the range 1-8**.

- If it is **invalid** (less than 0 or 9 or more), report it with `assert`
- If it is **valid**, return 1
- If it is **invalid**, return 0

```c
int validate_port_id(uint8_t port_id)
{
  assert(port_id >= 1 && port_id <= 8);
  return port_id >= 1 && port_id <= 8 ? 1 : 0;
}
```

**Important: the argument of a macro is evaluated twice, so notice that it is checked several times.**

## Macro traps

### The problem with the squaring macro `SQUARE(x)`

```c
#define SQUARE(x) x * x  // NG: missing parentheses
```

In this case, `SQUARE(1 + 2)` becomes `1 + 2 * 1 + 2 = 5`, not `(1 + 2) * (1 + 2) = 9`. Be careful about operator precedence.

### The problem with the multi-statement macro `DOUBLE_SQ(x)`

```c
#define DOUBLE_SQ(x) (x) = (x) * (x)  // NG: a syntax error when used after an if
```

```c
if (condition)
  DOUBLE_SQ(x);  // this can become a syntax error
```

**Solution**: wrap it in `do { ... } while (0)`. Then it is treated as a single statement.

```c
#define DOUBLE_SQ(x) do { (x) = (x) * (x); } while (0)
```

## About `_Static_assert`

This is a **compile-time assertion** introduced in C11. If the condition is false, it produces a compile error.

```c
_Static_assert(sizeof(CanPayload) == 8, "CanPayload must be 8 bytes");
```

**Note**: Unlike `assert`, it is checked at **compile time**, not at run time. It does not disappear in a release build.

## Run it

```bash
./drill run c12
```

**The tests are red until you start.** Since the header is not editable, the tests are set up to fail.

## Common pitfalls

- **The argument of a macro is evaluated several times**: `SQUARE(i++)` becomes `i++ * i++`. Do not pass an expression with side effects
- **Do not forget parentheses**: Wrap the macro arguments and the whole result in parentheses
- **Wrap a multi-statement macro in `do { ... } while (0)`**: This makes the behavior after the semicolon correct
- **`_Static_assert` is compile time**: It works the same way in debug builds and release builds
- **The port ID is 1-8**: 0 and 9 are invalid

## Tests

| Test | What it checks |
| --- | --- |
| `CanPayloadSizeIs8Bytes` | Whether the struct size is 8 bytes |
| `CanPayloadIsDefinedCorrectly` | Whether member access works |
| `DebugModeIsEnabled` | Whether `is_debug_mode()` returns 1 |
| `ValidatesPortId1AsValid` | Whether 1 is valid |
| `ValidatesPortId8AsValid` | Whether 8 is valid |
| `ValidatesPortId5AsValid` | Whether 5 is valid |
| `ValidatesPortId0AsInvalid` | Whether 0 is invalid |
| `ValidatesPortId9AsInvalid` | Whether 9 is invalid |
| `SquareOfOne` | Whether the macro works (basic) |
| `SquareOfFive` | Whether the calculation result is exact |
| `SquareOfNegativeNumber` | Handling of negative numbers |
| `DoubleSquareSquaresVariable` | Whether the multi-statement macro works |
| `DoubleSquareOfZero` | Handling of zero |
| `DoubleSquareCalledRepeatedly` | Consecutive calls |

## References

- [C track: 12. The preprocessor and debugging](../../docs-en/c/12_preprocessor_and_debugging.md)
