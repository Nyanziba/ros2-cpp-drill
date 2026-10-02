# cppb09 Count the copies [C++ Basics]

Learn value semantics and the efficiency of copying.

## What to do

Implement the following in `src/data.cpp`.

1. Copy constructor: increment `copy_ctor_count`
2. Copy assignment operator: increment `copy_assign_count`
3. `process_by_const_ref()`: take the argument as `const Data &` (avoid a copy)

## Run it

```bash
./drill run cppb09
```

## Common pitfalls

- The copy constructor and copy-assign are different. The tests count them separately.
- Using a `const &` parameter avoids copies.
- With RVO (return value optimization), the copy on return may disappear.

## Tests

```bash
./drill run cppb09
```

| Test | What it checks |
| --- | --- |
| `CopyConstructorIsCounted` | copy constructor |
| `CopyAssignmentIsCounted` | copy assignment operator |
| `PassByValueCopiesTwice` | the cost of pass by value |
| `PassByConstRefDoesNotCopy` | the benefit of const& |

## References

- [9. Value semantics](../../docs-en/cpp-basics/09_value_semantics.md)
