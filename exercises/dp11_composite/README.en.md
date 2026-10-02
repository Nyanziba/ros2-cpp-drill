# dp11 Composite [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 11. You express the robot's start-up self-diagnosis as a tree that **treats individual diagnoses (leaves) and groups (nodes) the same way**.
`File` / `Directory` / `Entry` correspond to `DiagnosticCheck` / `DiagnosticGroup` / `DiagnosticEntry`.

The theme of this exercise is ownership. The children are held in
`std::vector<std::unique_ptr<DiagnosticEntry>>`.
**You write not a single line of `delete`.**

## What to do

Implement 7 things in `src/diagnostic_tree.cpp`. The destructor is already implemented.

1. **`DiagnosticCheck::check_count()`**
   - A leaf is "one diagnosis that actually runs"

2. **`DiagnosticCheck::run()`**
   - If `passes_` is `true`, `passed = 1`; if `false`, `failed = 1`

3. **`DiagnosticCheck::collect_names()`**
   - Push `prefix + "/" + name()` to `out`

4. **`DiagnosticGroup::add()`**
   - `children_.push_back(std::move(child))`
   - `child` is a **named variable**, so if you forget `std::move`,
     you get a compile error saying "コピーコンストラクタは削除されています" (the copy constructor is deleted)
   - If `child` is `nullptr`, do nothing

5. **`DiagnosticGroup::check_count()`**
   - The sum of `check_count()` of the children. The group itself is not counted

6. **`DiagnosticGroup::run()`**
   - Call `run()` of the children in registration order and add up `passed` / `failed`

7. **`DiagnosticGroup::collect_names()`**
   - Push its own full path, and then recurse into the children using it as `prefix`

You do not need to use `dynamic_cast` to tell whether it is a leaf or a group. **Not telling them apart is Composite.**

## Run it

```bash
./drill run dp11
```

## Common pitfalls

- If you get `error: call to implicitly-deleted copy constructor of 'std::unique_ptr<...>'`,
  `push_back` is missing a `std::move`. A `unique_ptr` cannot be copied
- After `add(std::move(group))`, `group` is `nullptr`. You can never use it again
- You may want to hold the children in `std::vector<DiagnosticEntry>` (by value), but you cannot.
  It is an abstract class, so it cannot go into a `vector`, and even if it could,
  it would **slice** and the `DiagnosticCheck` part would disappear
- `DiagnosticGroup` has a `unique_ptr` member, so it **cannot be copied**.
  The `static_assert` at the top of the test checks this
- We do not give it a pointer to the parent. If parent and child refer to each other with `shared_ptr`,
  **they form a cycle and are never freed** (the article has a measurement in 11.5)

## Tests

```bash
./drill run dp11
```

There are 9 tests. Besides the recursive totals,
they check with the destructor log that **when you destroy the parent, the children are destroyed too**,
and check **not copyable, movable** with `static_assert`.

## References

- [11. Composite](../../docs-en/patterns/11_Composite.md)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- [cppreference: std::move](https://en.cppreference.com/w/cpp/utility/move)
- [cppreference: object slicing](https://en.cppreference.com/w/cpp/language/derived_class)
