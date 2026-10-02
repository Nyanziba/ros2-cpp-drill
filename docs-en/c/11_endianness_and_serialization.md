# 11. Endianness and serialization

> **Goal of this chapter**: In communication and log storage that handle byte sequences, it is important to decide "from which byte you pack a value that has several bytes (`float` or `uint32_t`)". After you understand the difference between little endian and big endian, and how to measure it, you learn how to convert safely between values and byte sequences with `memcpy`. The purpose is to understand why our team's CAN protocol implementation uses this pattern: "avoid pointer casts (`*(uint32_t *)&f`), pay attention to the signedness of `char`, and use `memcpy`".

## 11.1 What is byte order?

**Which end of memory does a multi-byte integer start from?**

When `uint32_t x = 0x12345678;` is stored in memory, the four bytes are laid out like this.

Little endian (the low-order byte comes first):
```
x[0] = 0x78, x[1] = 0x56, x[2] = 0x34, x[3] = 0x12
```

Big endian (the high-order byte comes first):
```
x[0] = 0x12, x[1] = 0x34, x[2] = 0x56, x[3] = 0x78
```

**In communication and file storage, if you do not make clear which byte order you send to the other side, it will not work.** Our team's CAN protocol states in its specification that it is "fixed little endian".

## 11.2 Measure your own environment

Instead of a pointer cast (which risks undefined behavior), convert a value to bytes safely with `memcpy` and find out which byte order it is.

**The basics of "packing one value into several bytes"**

```c
#include <stdint.h>
#include <string.h>

uint32_t val = 0x12345678;
unsigned char bytes[4];

/* unsafe: *(uint32_t *)&bytes[0] = val;  ← strict aliasing violation */
memcpy(bytes, &val, 4);  /* ← this is the right way */

printf("%02x %02x %02x %02x\n", bytes[0], bytes[1], bytes[2], bytes[3]);
```

Measured values:
```
78 56 34 12
```

`bytes[0]` is `0x78` (the lowest byte), so **this environment is little endian**.

## 11.3 Packing and extracting multi-byte values

**In little endian, use shifts and masks to arrange the bytes explicitly.**

### Build a uint32_t from a byte sequence

```c
uint32_t read_uint32_le(const unsigned char data[4]) {
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}
```

Byte [0] is the lowest and byte [3] is the highest.

### Split a uint32_t into a byte sequence

```c
void write_uint32_le(uint32_t value, unsigned char out[4]) {
    out[0] = (unsigned char)(value & 0xFF);
    out[1] = (unsigned char)((value >> 8) & 0xFF);
    out[2] = (unsigned char)((value >> 16) & 0xFF);
    out[3] = (unsigned char)((value >> 24) & 0xFF);
}
```

Extract each byte with a right shift and a mask, and pack it into `unsigned char` with a cast.

## 11.4 A pointer cast violates strict aliasing

**"Type punning" — the dangerous way and the right way.**

A common (**bad**) pattern:

```c
float f = 1.0f;
uint32_t u32_bad = *(uint32_t *)&f;  /* undefined behavior: lying about the type */
```

Under C's strict aliasing rule, **reading `float` memory by casting it to `uint32_t *` is undefined behavior**. The compiler does not notice this bug during optimization, and you get unexpected values or run-time errors.

**The right way: use `memcpy`.**

```c
float f = 1.0f;
uint32_t u32_good;
memcpy(&u32_good, &f, sizeof(f));  /* safe: byte-wise copy */
```

`memcpy` copies the value byte by byte, so it is not bound by the type system. The compiler can also optimize safely.

## 11.5 Handling the signedness of char

**When converting from a byte sequence to a wider value, pass it through `(unsigned char)` before widening.**

In C, whether `char` is signed or unsigned depends on the implementation. When a byte value is `0x80` or more, reading it as `char` makes it look negative, and widening it to `uint32_t` fills the upper bits with `1` by sign extension.

**An example broken by sign extension**

Reading the byte sequence `0x80, 0xFF, 0x00, 0x00` into a `uint32_t`:

```c
unsigned char raw[4] = {0x80, 0xFF, 0x00, 0x00};

/* bad example: cast with (char) and then widen */
uint32_t bad = (char)raw[0] |
               ((uint32_t)(char)raw[1] << 8) |
               ((uint32_t)(char)raw[2] << 16) |
               ((uint32_t)(char)raw[3] << 24);
printf("0x%08x\n", bad);  /* result: 0xffffff80 (corrupted by sign extension) */

/* good example: pass through (unsigned char) and then widen */
uint32_t good = (unsigned char)raw[0] |
                ((uint32_t)(unsigned char)raw[1] << 8) |
                ((uint32_t)(unsigned char)raw[2] << 16) |
                ((uint32_t)(unsigned char)raw[3] << 24);
printf("0x%08x\n", good);  /* result: 0x0000ff80 (correct) */
```

Measured value (bad example): `0xffffff80`  
Measured value (good example): `0x0000ff80`

**The difference is `0xFF000000` — the upper 3 bytes were filled with 1 by sign extension.**

## 11.6 Learning from our team's CAN implementation

In our team's motor driver and CAN protocol implementation (`TEXNITIS_CAN`), a `float` such as a target speed is packed into a CAN message. The specification is "Byte 0 = Port ID, Bytes 1-4 = float (LE), Bytes 5-7 = zero padding".

The core functions of the implementation (the C++ original, rewritten in C):

```c
/* pack a float into a byte sequence in little endian */
void write_float_le(float value, unsigned char data[8], size_t offset) {
    uint32_t u32;
    memcpy(&u32, &value, sizeof(value));  /* ← no type punning, safe with memcpy */

    data[offset + 0] = (unsigned char)(u32 & 0xFF);
    data[offset + 1] = (unsigned char)((u32 >> 8) & 0xFF);
    data[offset + 2] = (unsigned char)((u32 >> 16) & 0xFF);
    data[offset + 3] = (unsigned char)((u32 >> 24) & 0xFF);
}

/* read a float from a byte sequence in little endian */
float read_float_le(const unsigned char data[8], size_t offset) {
    uint32_t u32 = (unsigned char)data[offset + 0] |
                   ((uint32_t)(unsigned char)data[offset + 1] << 8) |
                   ((uint32_t)(unsigned char)data[offset + 2] << 16) |
                   ((uint32_t)(unsigned char)data[offset + 3] << 24);

    float f;
    memcpy(&f, &u32, sizeof(f));  /* ← no type punning, safe with memcpy */
    return f;
}
```

**Why this implementation passes through `(unsigned char)`:** The `data` array is declared as `char`, so there is a risk of sign extension when each element is read. The `(unsigned char)` cast makes sure it is zero-extended.

**Why it uses `memcpy`:** `float` and `uint32_t` are originally unrelated types. If you lie about the type with a pointer cast, you break the compiler's optimization rule (strict aliasing) and get unexpected behavior. `memcpy` is a copy of a byte sequence, so it is safe independently of the type system.

**Round-trip test (verifying our team's implementation):**

```c
float original = 3.14f;
unsigned char data[8] = {0};

write_float_le(original, data, 1);
float recovered = read_float_le(data, 1);

if (original == recovered) {
    printf("Round trip succeeded\n");
}
```

Measured:
```
元の値: 3.14
バイト列: c3 f5 48 40 
復元値: 3.14
→ 往復一致
```

## Try it yourself

Copy the following, compile and run it, and check it in your own environment too.

**Predict: In little endian, the byte sequence of `1.0f` should be `00 00 80 3f`.**

```c
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void write_uint32_le(uint32_t value, unsigned char out[4]) {
    out[0] = (unsigned char)(value & 0xFF);
    out[1] = (unsigned char)((value >> 8) & 0xFF);
    out[2] = (unsigned char)((value >> 16) & 0xFF);
    out[3] = (unsigned char)((value >> 24) & 0xFF);
}

uint32_t read_uint32_le(const unsigned char data[4]) {
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

void write_float_le(float value, unsigned char out[4]) {
    uint32_t u32;
    memcpy(&u32, &value, sizeof(value));
    write_uint32_le(u32, out);
}

float read_float_le(const unsigned char data[4]) {
    uint32_t u32 = read_uint32_le(data);
    float f;
    memcpy(&f, &u32, sizeof(f));
    return f;
}

int main(void) {
    printf("=== Endianness test ===\n");
    uint32_t val = 0x12345678;
    unsigned char bytes[4];
    write_uint32_le(val, bytes);
    printf("0x%08x -> %02x %02x %02x %02x\n", val,
           bytes[0], bytes[1], bytes[2], bytes[3]);

    printf("\n=== float <-> bytes ===\n");
    float f = 1.0f;
    unsigned char fbytes[4];
    write_float_le(f, fbytes);
    printf("%.1f -> %02x %02x %02x %02x\n", f,
           fbytes[0], fbytes[1], fbytes[2], fbytes[3]);

    float recovered = read_float_le(fbytes);
    printf("recovered: %.1f\n", recovered);

    printf("\n=== Sign extension test ===\n");
    unsigned char raw[4] = {0x80, 0xFF, 0x00, 0x00};
    uint32_t u32_good = read_uint32_le(raw);
    printf("Byte sequence %02x %02x %02x %02x -> 0x%08x\n",
           raw[0], raw[1], raw[2], raw[3], u32_good);

    return 0;
}
```

Result (measured):
<details markdown="1"><summary>Answer (actual output)</summary>

```
=== Endianness test ===
0x12345678 -> 78 56 34 12

=== float <-> bytes ===
1.0 -> 00 00 80 3f
recovered: 1.0

=== Sign extension test ===
Byte sequence 80 ff 00 00 -> 0x0000ff80
```

</details>

## Common pitfalls

### 1. The temptation to "read a byte sequence with `*(uint32_t *)&bytes[0]`"

```c
/* bad example */
uint32_t val = *(uint32_t *)&bytes[0];  /* strict aliasing violation */
```

Depending on the compiler this may work, but it often breaks when you raise the optimization level.

**Diagnostic message:** The compiler often does not report the violation, but unexpected values appear at run time.

**Fix:** Use `memcpy`.

```c
uint32_t val;
memcpy(&val, bytes, sizeof(val));  /* safe */
```

### 2. Values break because of the signedness of `char`

```c
char data[8];
/* ... in the receive handling, data[0] = 0x80 and so on ... */

uint32_t bad = (char)data[0] |
               ((uint32_t)(char)data[1] << 8) |
               ...;  /* the upper bits are filled with 1 by sign extension */
```

**Fix:** Pass it through `(unsigned char)`.

```c
uint32_t good = (unsigned char)data[0] |
                ((uint32_t)(unsigned char)data[1] << 8) |
                ...;  /* the upper bits are 0 by zero extension */
```

### 3. You are unsure when packing or reading a `float` as bytes

If you want to see "how a `float` is laid out in memory", copy it with `memcpy`, not with a pointer cast, and look at it as a `uint32_t`. The other direction is the same.

```c
float f = 3.14f;
uint32_t u32;
memcpy(&u32, &f, 4);  /* get the memory image of the float as a uint32_t */
printf("0x%08x\n", u32);
```

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c11_endian_serialize` — endianness and serialization

```bash
./drill run c11
```

If you get stuck, use `./drill hint c11`. From the exercise side, `./drill read c11` brings you back to this chapter.

## References

- C99 standard (N1256): 6.2.5 Types, Annex J (Portability issues): Unspecified behavior
- cppreference.com: [std::memcpy](https://en.cppreference.com/w/c/string/byte/memcpy)
- GCC Manual: [Warnings and Errors Related to Undefined Behavior](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)

---

Previous chapter → [10. volatile and interrupt safety](10_volatile_and_interrupt_safety.md)

Next chapter → [12. Preprocessor and debugging](12_preprocessor_and_debugging.md)
