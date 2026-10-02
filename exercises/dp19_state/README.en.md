# dp19 State [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 19. You implement the robot's operating state machine **in 3 ways with the same rules** and compare them.

## Transition rules

```
Stopped --PowerOn--> Idle --Start--> Running --Stop--> Idle
From any state, EmergencyStop --> Faulted
Faulted --Reset--> Stopped          (you can leave only by a manual reset)
Combinations not in the table are ignored. The state does not change, and no entry/exit actions run.
```

Entry and exit actions (all 3 implementations must produce the same sequence):

- Exit: `"exit:<state name>"`. **Only when leaving Running**, followed by `"motor:stop"`
- Entry: `"enter:<state name>"`. **Only when entering Faulted**, followed by `"brake:engage"`
- **Exit first, entry after**

## What to do

Implement 3 ways in `src/state_machine.cpp`.

### Way 1: `enum` + `switch`

1. **`EnumStateMachine::next_state()`** — a static function that just looks up the transition table. No side effects
2. **`EnumStateMachine::handle()`** — only when it transitions, `log_exit` → swap → `log_enter`

Do not write `default:` in the `switch`. If you list all the `enum` values,
**the compiler warns you about what you forgot to write** when you add a state later.

### Way 2: State classes (GoF version)

3. **`handle()` of the 4 `State` derived classes** — just return a `const State *` for the destination.
   If it does not transition, return `this`
4. **Override `RunningStateObject::on_exit()` / `FaultedStateObject::on_enter()`**
5. **`state_object()`** — returns the `static` instance that corresponds to a state id
6. **`ClassStateMachine::handle()`** — swap **after you have received** the destination

### Way 3: `std::variant` + `std::visit`

7. **`id_of()`** — convert with `std::visit` and `if constexpr`
8. **`VariantStateMachine::next_state()`** — return the destination. If it does not transition, `std::nullopt`
9. **`VariantStateMachine::handle()`**

## Run it

```bash
./drill run dp19
```

## Common pitfalls

- **Do not swap the state machine inside `handle()`.** To make that impossible in the first place,
  `State::handle()` does not take a `Context`. This is the most important point of this chapter.
  If you copy the Java version's `context.changeState(...)` into C++ word for word, it becomes `delete this`
- **Do not run the actions on an input that does not transition.** If `on_exit` → `on_enter` run when `Running` receives `Start` again,
  on the real robot the motor stops for a moment and restarts
- **Exit first, entry after.** If you reverse it, the motor stop runs after the brake is applied
- `state_object()` must return **the same address** no matter how many times you call it.
  A state object has no members at all, so one `static` instance is enough (zero heap allocations)
- If the return type differs for each branch in the lambda of `std::visit`, it is an error.
  Write the return type explicitly, like `-> std::optional<StateVariant>`
- `EmergencyStop` means "to Faulted from any state", but **if it is already Faulted, it does not transition**
  (that is, `false` is returned, and `brake:engage` does not run twice)

## Tests

```bash
./drill run dp19
```

There are 13 tests. They check all 20 combinations of the transition table, the order of the actions,
that the state object of the source is still alive after a transition, that the `variant` version is not polymorphic,
and that **the 3 implementations return the same sequence of transitions and the same log**.

## References

- [19. State](../../docs-en/patterns/19_State.md)
- [C track 9. Function pointers](../../docs-en/c/09_function_pointers.md) — the same tool as the table-driven version for microcontrollers
- [cppreference: std::variant](https://en.cppreference.com/w/cpp/utility/variant)
- [cppreference: std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
