# c07 Bit operations and register access [C track]

In this exercise you use bit operations to build and split a CAN ID, and to implement a register read-modify-write.

## The header is already complete

You **do not edit** `include/drill/bit_ops.h`. The function interfaces for the CAN ID and register operations are defined in it.

## CAN ID specification

An 11-bit ID is made of three fields:
- `[10:8]` **ReceiverType** (3 bit) — the target of the message
  - 0=ALL, 1=MD, 2=BLDC, 3=OTHER_ACTUATOR, 4=SENSOR, 5=MICROCONTROLLER, 6=MAINBOARD_PC, 7=UNDEFINED
- `[7:4]` **DeviceId** (4 bit) — device number (0-15)
- `[3:0]` **Command** (4 bit) — command (0-15)

## What to do

The only file you edit is `src/bit_ops.c`. Implement the following functions:

### CAN ID functions
- `can_id_create(type, device, cmd)` — build an 11-bit ID from the three fields
- `can_id_extract_type(id)` — extract the Type from the ID
- `can_id_extract_device(id)` — extract the Device from the ID
- `can_id_extract_cmd(id)` — extract the Cmd from the ID
- `can_id_set_device(id, new_device)` — change only the Device with **Read-Modify-Write** (do not break the others)

### Register functions
- `register_set_bit(reg, bit_pos)` — set the given bit to 1
- `register_clear_bit(reg, bit_pos)` — set the given bit to 0
- `register_read_bit(reg, bit_pos)` — read the given bit
- `register_set_bits(reg, msb, lsb, value)` — set the bit range [msb:lsb] to value

## Run it

```bash
./drill run c07
```

## Common pitfalls

- **Shift operations** — `(value << n)` shifts left by n bits. Note that a left shift widens the bit width (`uint8_t` to `uint16_t`)
- **Masks** — To take out a bit range, filter with `(value >> shift) & mask`
- **Read-Modify-Write** — In `can_id_set_device`, you must preserve everything except the Device field
  - Wrong implementation: `(new_device << 4)` — Type and Cmd are lost
  - Correct implementation: `(id & ~0xF0) | (new_device << 4)` — clear the Device and add the new value
- **Setting a bit range** — `register_set_bits` needs a mask calculation

## Tests

| Test | What it checks |
| --- | --- |
| `組み立てと抽出が往復する` (building and extracting round-trip) | The basics of bit operations (shifts and masks) |
| `ReadModifyWrite_他のビットを壊さない` (ReadModifyWrite does not break the other bits) | The read-modify-write pattern |
| `ビットを立てる/落とす` (set / clear a bit) | Single-bit register operations |
| `ビット範囲を設定` (set a bit range) | Read-modify-write of several bits |

## References

- [7. Bit operations and register access](../../docs-en/c/07_bit_operations_and_registers.md)
