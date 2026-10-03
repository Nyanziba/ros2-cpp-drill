# c08 Manual memory management [C track]

This exercise teaches the basics of dynamic memory management with `malloc` and `free`.

## The header is already complete

You **do not edit** `include/drill/malloc_free.h`.
The interfaces for the dynamic array and the linked list are defined in it.

## What to do

The only file you edit is `src/malloc_free.c`. Implement the following functions:

### Dynamic array
- `create_array(size)` — allocate an `int` array with `malloc`
- `free_array(arr)` — release the array with `free`

### Linked list
- `create_linked_list(length)` — create a linked list of length `length`
  - The `value` of each node is 0, 1, 2, ..., length-1
  - Return `NULL` if `malloc` fails
- `free_linked_list(head)` — release the list recursively

## Run it

```bash
./drill run c08
```

## Common pitfalls

- **Handling malloc failure** — `malloc` can return `NULL`. You must check it
- **use-after-free** — When `malloc` fails in `create_linked_list`, the nodes already allocated must not leak (release them with `free_linked_list`)
- **Recursive free** — `free_linked_list` is recursive, but it is important to **save `next` first** and then `free(head)`
  - Wrong implementation: `free(head); free_linked_list(head->next);` → use-after-free
  - Correct implementation: `struct Node * next = head->next; free(head); free_linked_list(next);`

## AddressSanitizer (ASAN)

`-fsanitize=address` is enabled for these tests.

- **The tests pass, but the drill is red** ← memory is leaking
  - Check that you `free` every node
- **A leak is reported** ← read the output of the leak sanitizer and investigate the cause

## Tests

| Test | What it checks |
| --- | --- |
| `AllocatesArray` | Whether the array is allocated with `malloc` and can be accessed |
| `WorksWithDifferentSizes` | Whether several arrays can be managed independently |
| `CreatesList` | Whether a linked list can be created with `malloc` |
| `ManagesMultipleListsIndependently` | Whether the memory is not mixed up |

## References

- [8. Manual memory management](../../docs-en/c/08_manual_memory_management.md)
