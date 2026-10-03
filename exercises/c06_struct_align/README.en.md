# c06 Structs and alignment [C track]

This exercise helps you understand how a struct is laid out in memory and why padding appears.

## The header is already complete

You **do not edit** `include/drill/struct_align.h`. Three structs are defined in it:

```c
struct Point2D {
  int16_t x;
  int16_t y;
};

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct PackedData {
  uint8_t flag;
  uint64_t id;
  uint16_t counter;
};
```

## What to do

The only file you edit is `src/struct_align.c`.
In each function, **return the `sizeof` and `offsetof` of the matching struct.**

```c
size_t get_sizeof_point2d(void);        // sizeof(struct Point2D)
size_t get_offset_point2d_x(void);      // offsetof(struct Point2D, x)
size_t get_offset_point2d_y(void);      // offsetof(struct Point2D, y)
// ... the same for RGB and PackedData
```

Use the `offsetof` macro from the C99 `<stddef.h>`.

## Run it

```bash
./drill run c06
```

## Common pitfalls

- **Alignment** — The CPU reads values from specific address boundaries, so the offset of each member is often a multiple of the size of its type
- **Padding** — Gaps are inserted between members, and `sizeof` can exceed "the sum of the sizes of all members"
- `struct PackedData` mixes small members and large members. Pay attention to the order

## Tests

| Test | What it checks |
| --- | --- |
| `Point2dSizeIs4Bytes` | How many bytes a struct of two int16_t becomes |
| `RgbSizeIs3Bytes` | The case of three uint8_t (no padding) |
| `PackedDataSizeIs24Bytes` | How padding is inserted when small and large types are mixed |
| `Point2dXOffsetIs0` and the other 7 `*OffsetIs*` tests | At which byte position each member is actually placed |

## References

- [6. Structs and alignment](../../docs-en/c/06_structs_and_alignment.md)
