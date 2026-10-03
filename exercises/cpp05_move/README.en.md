# c05 Move ownership [C++]

Learn move semantics.

## What to do

Implement the move constructor and the move assignment in `src/buffer.cpp`.

The Buffer class manages a dynamic array. Copying is prohibited (deleted), and only moving is allowed.

Functions to implement:
- Move constructor: `Buffer(Buffer&& other) noexcept`
- Move assignment: `Buffer& operator=(Buffer&& other) noexcept`

## Run it

```bash
./drill run c05
```

## Common pitfalls

- After the move, the original object is in an "empty" state.
  Set data_ to nullptr and size_ to 0.
- In the move assignment, free the existing memory first, and then take over the other object's resources.
- Do not forget the self-assignment check `if (this != &other)`.

## Tests

```bash
./drill run c05
```

| Test | What it checks |
| --- | --- |
| `MoveConstructorTransfersData` | pointer transfer in the move constructor |
| `MoveAssignmentTransfersData` | a correct implementation of the move assignment |

## References

- [cppreference: Move semantics](https://en.cppreference.com/w/cpp/language/move)
- [cppreference: rvalue reference](https://en.cppreference.com/w/cpp/language/rvalue_reference)
- [5. Move ownership](../../docs-en/cpp/05_move_and_ownership.md)
