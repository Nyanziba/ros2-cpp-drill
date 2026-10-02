# dp13 Visitor [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 13. For the result tree of the start-up self-check,
you implement 2 ways, the **GoF double dispatch** and **`std::variant` + `std::visit`**,
and check that they give the same result.

The tree we handle is this.

```
[selftest]
  bat 11800mV OK
  [drive]
    motor_l fault=0 OK
    motor_r fault=3 NG
  temp 65000mV NG
```

## What to do

Implement it in `src/diagnostics.cpp`. Do not edit the header.

### Part 1: GoF version (double dispatch)

1. **`SensorCheck::accept` / `MotorCheck::accept` / `CheckGroup::accept`**
   - Each is one line: `visitor.visit(*this);`
   - **The three look the same, but you cannot write one in the base class to cover them.**
     Inside the base class, the static type of `*this` is `DiagNode`, and there is no `visit` that takes it
2. **`CheckGroup::add`** — put it into `children_` with `std::move` (the group owns it)
3. **`FailureCountVisitor::visit` (3 functions)** — count at the leaves, and at the group `accept` the children
4. **`TextReportVisitor::visit` (3 functions)** — at the group, add lines while increasing and decreasing `depth_`

**Walking the tree is the visitor's job** (the same as `ListVisitor` in Yuki's book).
If you write the traversal on the element side, you cannot write a visitor that wants to change the visiting order.

### Part 2: `std::variant` version (no inheritance, no virtual functions, no `accept`)

5. **`DiagArena::add`** — put it at the end and return the index
6. **The `overloaded` idiom** — write it yourself in an anonymous namespace

   ```cpp
   template <class ... Ts>
   struct overloaded : Ts ...
   {
     using Ts::operator() ...;
   };

   template <class ... Ts>
   overloaded(Ts ...) -> overloaded<Ts ...>;
   ```

   In C++17 **you have to write the deduction guide (the lower 2 lines) yourself**. In C++20 it is not needed
7. **`count_failures` / `make_report`** — branch by kind with `std::visit`.
   At a group, follow the indexes of the children and recurse.
   Return **a string that does not differ by one character from the GoF version**

The output format (the indentation and the format of the lines) is prepared in `diag_format` of
`include/drill/diagnostics.hpp`. If both versions use it, the strings do not drift apart.

## Run it

```bash
./drill run dp13
```

## Common pitfalls

- While you have not implemented `accept`, the tests of the `variant` version fail with
  `C++ exception with description "vector"`.
  `DiagArena::add` does not save anything, so `at()` is just out of range
- If `accept` is not virtual, the kind disappears when you call through a base pointer.
  The test "基底ポインタ経由でも派生ごとのvisitが選ばれる" (the visit of each derived class is chosen even through a base pointer) checks that
- If you make the argument of `visit` `SensorCheck` instead of `const SensorCheck &`, a **copy** happens.
  If you take it as the base type, it **slices**
- If you forget the deduction guide of `overloaded`, you get
  `no viable constructor or deduction guide for deduction of template arguments`
- A lambda cannot call itself recursively.
  Prepare a named function in an anonymous namespace and do `std::visit` inside it
- If even one lambda passed to `std::visit` is missing, it is a **compile error**.
  This is not an accident; it means **it tells you the places to fix when you add a kind**

## Tests

```bash
./drill run dp13
```

There are 11 tests. They check that double dispatch works,
that 2 kinds of Visitor can be applied to the same tree, that the `variant` version matches the GoF version,
and that the `variant` version is not polymorphic (`static_assert`).

## References

- [13. Visitor](../../docs-en/patterns/13_Visitor.md)
- [cppreference: std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
- [cppreference: std::variant](https://en.cppreference.com/w/cpp/utility/variant)
