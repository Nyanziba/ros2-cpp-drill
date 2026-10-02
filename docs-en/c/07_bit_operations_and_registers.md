# 7. Bit operations and register access

> **Goal of this chapter**: In microcontroller control, you read and write individual bits of registers. The basic technique is to combine the C bit operators `&`, `|`, `^`, `~`, `<<`, and `>>` so that you can "set / clear / read one bit" without "breaking the other bits". Our team's CAN protocol builds an 11-bit ID with `(type << 8) | (device << 4) | cmd` and splits it apart again, and it uses the same logic.

## 7.1 Basic bit operations

**Set a bit with OR `|`.**

```c
uint8_t x = 0;
x |= (1 << 2);    // set bit 2
```

`1 << 2` is `0b00000100` (= `0x04`), so `x` becomes `0x04`.

| Expression | Binary | Hex |
| --- | --- | --- |
| `x` (initial value) | `00000000` | `0x00` |
| `1 << 2` | `00000100` | `0x04` |
| Result of `x \|= (1 << 2)` | `00000100` | `0x04` |

**Read a bit with AND `&` (mask).**

```c
int bit2 = (x >> 2) & 1;  // read bit 2
```

Shift `x` right by `2` to bring bit 2 back to the lowest position, then `& 1` takes out only the lowest bit.

**Clear a bit with AND + NOT `&= ~()` (read-modify-write).**

```c
x &= ~(1 << 2);   // clear bit 2 (the other bits do not change)
```

Starting from the state where bit 5 is also set (`0x24`), it goes like this.

| Expression | Binary | Hex |
| --- | --- | --- |
| `x` (bits 2 and 5 are set) | `00100100` | `0x24` |
| `~(1 << 2)` | `11111011` | `0xFB` |
| Result of `x &= ~(1 << 2)` | `00100000` | `0x20` |

**Bit 5 remains.** This is the point of read-modify-write.

### Why read-modify-write matters

**A trick to avoid breaking the other bits.**

```c
// bad example: you want to set bit 5
x = (1 << 5);  // all the other bits become 0 ✗

// good example: set only bit 5
x |= (1 << 5);  // the existing bit 2 is kept ✓
```

## 7.2 A real example: building and splitting a CAN ID

**A CAN message shows the destination and the command with an 11-bit ID.**

- bit[10:8] — ReceiverType (receiver category)
- bit[7:4] — DeviceId (device number)
- bit[3:0] — Command (function code)

**Building:**

```c
uint8_t type = 1;      // ReceiverType::MD = 0x01
uint8_t device = 5;    // device 5
uint8_t cmd = 7;       // command 7

uint16_t can_id = (type << 8) | (device << 4) | cmd;
```

Measured values:

```
CAN ID: 0x157 (type=1, device=5, cmd=7)
```

Breakdown of `0x157`: `0x100` (type, 1 shifted left by 8 bits) `+ 0x50` (device, 5 shifted left by 4 bits) `+ 0x7` (cmd).

**Splitting:**

```c
uint8_t extracted_type = (can_id >> 8) & 0x07;
uint8_t extracted_device = (can_id >> 4) & 0x0F;
uint8_t extracted_cmd = can_id & 0x0F;
```

Measured values:

```
Extracted: type=1, device=5, cmd=7
```

**The meaning of the bit masks (`0x07`, `0x0F`).**

- `0x07` = `0b0111` = extracts 3 bits (bit[2:0])
- `0x0F` = `0b1111` = extracts 4 bits (bit[3:0])

Shift right to adjust the position, and take out only the number of bits you need.

## 7.3 Flip bits with XOR `^`

**XOR `^` is "0 if the values are the same, 1 if they differ". You can use it to flip bits.**

```c
uint8_t x = 0x0F;   // 0b00001111
x ^= 0x03;           // flip the lower 2 bits
                     // 0b00001100 = 0x0C
```

XOR is handy for "toggling a state".

```c
#define LED_PIN 3
uint32_t gpio_state = 0;
gpio_state ^= (1 << LED_PIN);  // flip the LED pin (on → off, off → on)
```

## 7.4 NOT `~` — be careful

**NOT `~` flips all the bits of the whole type.**

```c
uint8_t x = 0x01;     // 0b00000001
uint8_t y = ~x;       // 0b11111110 = 0xFF
```

**Be careful when you build a mask with `~(1 << 2)`.** `1` is an `int` by default, so its type may be widened in a shift operation.

To write it safely:

```c
uint8_t x = 0xFF;
x &= ~((uint8_t)(1 << 2));  // cast explicitly
```

## 7.5 The pitfall of signed shifts

**A right shift `>>` on a signed integer is an "arithmetic shift" (sign extension).**

```c
int8_t neg = -1;   // 0b11111111 (all bits are 1)
int8_t shifted = neg >> 1;  // 0b11111111 = -1 (sign-extended)
```

Measured values:

```
neg = -1, neg >> 1 = -1 (arithmetic right shift)
```

**With an unsigned type it is a "logical shift" (filled with 0).**

```c
unsigned char uneg = 255;  // 0b11111111
unsigned char ushifted = uneg >> 1;  // 0b01111111 = 127
```

Measured values:

```
255u >> 1 = 127 (logical right shift)
```

**Always do bit operations on `unsigned` types.** Otherwise the handling of the sign is uncertain.

## 7.6 Bit fields — the order depends on the implementation

**You can also make bit-sized regions with bit fields in a struct.**

```c
struct with_bitfield {
    unsigned int a : 3;   // 3 bits
    unsigned int b : 2;   // 2 bits
    unsigned int c : 3;   // 3 bits
} bf;

bf.a = 5;
bf.b = 3;
bf.c = 7;
```

**Warning: the memory layout depends on the implementation. Even with the same code, the order may differ between GCC and MSVC.**

In microcontroller control, **using bit operations directly** is more reliable than bit fields.

## Try it yourself

Check the basics of bit operations, building and splitting a CAN ID, and how shifts behave.

```c
// bitops_all.c
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    printf("== Basic bit operations ==\n");
    uint8_t x = 0;
    printf("x = 0x%02x\n", x);

    // Set bit 2
    x |= (1 << 2);
    printf("After x |= (1 << 2): x = 0x%02x\n", x);

    // Set bit 5
    x |= (1 << 5);
    printf("After x |= (1 << 5): x = 0x%02x\n", x);

    // Clear bit 2
    x &= ~(1 << 2);
    printf("After x &= ~(1 << 2): x = 0x%02x\n", x);

    // Read bit 5
    int bit5 = (x >> 5) & 1;
    printf("Bit 5 of x: %d\n", bit5);

    printf("\n== CAN ID: (type << 8) | (device << 4) | cmd ==\n");
    uint8_t type = 1;
    uint8_t device = 5;
    uint8_t cmd = 7;
    uint16_t can_id = (type << 8) | (device << 4) | cmd;
    printf("CAN ID: 0x%03x (type=%d, device=%d, cmd=%d)\n", can_id, type, device, cmd);

    // Extract
    uint8_t ext_type = (can_id >> 8) & 0x07;
    uint8_t ext_device = (can_id >> 4) & 0x0F;
    uint8_t ext_cmd = can_id & 0x0F;
    printf("Extracted: type=%d, device=%d, cmd=%d\n", ext_type, ext_device, ext_cmd);

    printf("\n== Signed shift (dangerous) ==\n");
    int8_t neg = -1;
    int8_t shifted = neg >> 1;
    printf("neg = %d, neg >> 1 = %d (arithmetic right shift)\n", neg, shifted);

    unsigned char uneg = 255;
    unsigned char ushifted = uneg >> 1;
    printf("255u >> 1 = %u (logical right shift)\n", ushifted);

    printf("\n== XOR for toggling ==\n");
    uint8_t gpio = 0;
    printf("gpio = 0x%02x\n", gpio);
    gpio ^= (1 << 3);
    printf("After gpio ^= (1 << 3): gpio = 0x%02x\n", gpio);
    gpio ^= (1 << 3);
    printf("After gpio ^= (1 << 3) again: gpio = 0x%02x\n", gpio);

    return 0;
}
```

**Predict: Does building the CAN ID give `0x157`? Does splitting it give back the original values? Does a right shift of a signed `int8_t` give a different result from a right shift of an unsigned `unsigned char`?**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic bitops_all.c -o bitops_all && ./bitops_all
```

<details markdown="1"><summary>Answer (actual output)</summary>

```
== Basic bit operations ==
x = 0x00
After x |= (1 << 2): x = 0x04
After x |= (1 << 5): x = 0x20
After x &= ~(1 << 2): x = 0x20
Bit 5 of x: 1

== CAN ID: (type << 8) | (device << 4) | cmd ==
CAN ID: 0x157 (type=1, device=5, cmd=7)
Extracted: type=1, device=5, cmd=7

== Signed shift (dangerous) ==
neg = -1, neg >> 1 = -1 (arithmetic right shift)
shifted = -1
255u >> 1 = 127 (logical right shift)

== XOR for toggling ==
gpio = 0x00
After gpio ^= (1 << 3): gpio = 0x08
After gpio ^= (1 << 3) again: gpio = 0x00
```

</details>

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. Splitting the CAN ID `0x157` gives back `type=1, device=5, cmd=7`.
2. For signed `int8_t(-1) >> 1` the result is `-1` (sign extension), and for unsigned `unsigned(255) >> 1` it is `127` (filled with 0).
3. XOR toggles when done twice (it returns to the original).

</details>

Next, try the following.

- Try the CAN ID with another bit pattern (for example `type=6, device=15, cmd=3`)
- Shift a `uint8_t` value with `1 << 8` and observe how it overflows the type

## Common pitfalls

**The result of a bit operation is different from what you expected**
Check the shift amount, the mask value, and the type. Also consider the possibility of sign extension.

**`x = (1 << bit)` clears the other bits**
Use the OR assignment `|=`, not the assignment `=`.

**The value stays negative after a right shift**
An arithmetic shift is done on a signed type. Use an `unsigned` type, or cast.

**The order of bit fields differs between platforms**
It depends on the implementation. In microcontroller control, using bit operations directly is safer.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c07_bit_ops` — bit operations and CAN IDs

```bash
./drill run c07
```

If you get stuck, use `./drill hint c07`. From the exercise side, `./drill read c07` brings you back to this chapter.

## References

- [cppreference: Bitwise operators](https://en.cppreference.com/w/c/language/operator_arithmetic)

---

Previous chapter → [6. Structs and alignment](06_structs_and_alignment.md)
Next chapter → [8. Manual memory management](08_manual_memory_management.md)
