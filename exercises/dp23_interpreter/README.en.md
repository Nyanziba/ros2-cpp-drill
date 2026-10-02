# dp23 Interpreter [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 23. You turn the motion description mini language **MotionScript** into a syntax tree and expand it into a sequence of motions.

```
forward 100; turn 90; repeat 3 { forward 50; }
```

↓

```
forward 100, turn 90, forward 50, forward 50, forward 50
```

## The grammar (this is all of it)

```
program   := statement*
statement := "forward" NUMBER ";"
           | "turn"    NUMBER ";"
           | "repeat"  NUMBER "{" program "}"
```

## What to do

Implement 5 things in `src/motion_script.cpp`. The lexical analysis (`tokenize`) is already implemented.

1. **`CommandNode::evaluate()`**
   - Push the one `Motion` it holds onto `out`

2. **`SequenceNode::evaluate()`**
   - Call `evaluate()` on the children from the first one in order

3. **`RepeatNode::evaluate()`**
   - Call `evaluate()` on the body `count_` times

4. **`parse()`** — a recursive descent parser
   - It is straightforward to split it into `parse_sequence(depth)` / `parse_statement(depth)`
   - **Do not throw exceptions.** Return `ParseResult::failure(ParseError{...})`
   - Reject `depth > kMaxNestingDepth` at the entrance of `parse_sequence`

5. **`run_variant()`** — evaluation with the `std::variant` + `std::visit` version
   - It must give the same result as `run()` of the class version

## Run it

```bash
./drill run dp23
```

## Common pitfalls

- **`evaluate()` does not return a result; it pushes onto `out`.** If you make a `std::vector` for each node and
  join them, allocations run as many times as the depth of the tree
- How to pass a failure upward. If you do not use `throw`, the standard way is
  **"put the error in a member, and the function returns `nullptr`"**
- `parse_sequence` stops at **both** `}` and the end of input.
  If you forget to stop, the body of `repeat` eats into the outside
- Check the depth limit at the **entrance of the recursion**. Even if you check at the exit or in clean-up,
  the stack runs out before you get there
- `variant_ast::VNode` is `std::variant<Command, std::unique_ptr<Repeat>>`.
  **It is a recursive type, so using a variant does not remove the indirection.**
  The elements of a variant must be complete types, so `Repeat` is wrapped in a `unique_ptr`
- The callable you pass to `std::visit` must have an **`operator()` for every candidate type**.
  If one is missing, you get dozens of lines of template errors

## Tests

```bash
./drill run dp23
```

There are 13 tests. They check the expansion of nesting, that syntax errors are returned as error values,
that the `std::variant` version gives the same result, and that it does not crash on too-deep nesting.

## References

- [23. Interpreter](../../docs-en/patterns/23_Interpreter.md)
- [cppreference: std::variant](https://en.cppreference.com/w/cpp/utility/variant)
- [cppreference: std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
