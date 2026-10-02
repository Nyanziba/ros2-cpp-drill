# dp22 Command [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 22. Using teleoperation commands for a robot arm as the subject, you implement Command in 3 ways.
**A class version (with undo / redo)**, **a `std::function` version (no undo)**, and
**a POD + fixed-length ring buffer version for microcontrollers**.

Writing all 3 is the point. You learn the criteria for "which one to choose".

## What to do

Implement it in `src/robot_console.cpp`. `RobotArm` in the header is already implemented.

1. **`RotateCommand::execute()` / `undo()` / `name()`**
   - A relative rotation. **The inverse operation is obvious** (the inverse of +30 is -30), so it does not save state
   - `name()` is `"rotate"`

2. **`GripperCommand::execute()` / `undo()` / `name()`**
   - An example where **the inverse operation is not obvious**. In `execute()`, keep the state before the execution in `previous_`
   - The inverse of "close" is not "open". If it was closed from the start, staying closed is the right answer
   - `name()` is `"grip"` if `closed_`, otherwise `"release"`

3. **`MacroCommand::add()` / `execute()` / `undo()` / `name()`**
   - It **owns** the children with `std::vector<std::unique_ptr<Command>>`
   - `execute()` is in the order they were added, and **`undo()` is in reverse order**
   - `MacroCommand` itself is also a `Command`, so you can put a macro inside a macro (Composite)

4. **`CommandHistory::run()` / `undo()` / `redo()`**
   - `run()` executes, pushes onto `done_`, and **discards `undone_`**
   - `undo()` cancels the end of `done_` and moves it to `undone_`
   - `redo()` executes the end of `undone_` and moves it back to `done_`

5. **`ActionQueue::push()` / `run_all()`**
   - The `std::function<void()>` version. A command queue that makes no class at all
   - Do not push an empty `std::function`

6. **`MotorCommandRing::push()` / `pop()` and `apply()`**
   - The microcontroller version with no dynamic allocation and no virtual functions
   - If it is full, **drop the oldest command** and return `false`
   - `apply()` uses a `switch` instead of `virtual`

## Run it

```bash
./drill run dp22
```

## Common pitfalls

- **The order of `undo()` is the main point.** If you undo a macro in forward order,
  the state breaks for operations that depend on each other (such as grasping and then lifting). Undo from the end
- **Do not forget `undone_.clear()` in `CommandHistory::run()`.**
  If you could redo after you undo, run another command, and then redo,
  you would get a state that cannot be restored
- `std::move` `done_.back()` and then `pop_back()`.
  If you reverse the order, you touch something that has already been released
- `add()` / `run()` / `push()` take a `std::unique_ptr` **by value**.
  If you take it with `const std::unique_ptr<Command> &`, it cannot be copied, and you cannot even push it
- **A command does not own the `RobotArm`.** What `RotateCommand` holds is
  a raw pointer. If it outlives the arm, it dangles (the same problem as chapter 17, Observer)
- `head_` of `MotorCommandRing` is "**the next position to write**".
  The oldest is `(head_ + kCapacity - size_) % kCapacity`.
  So that `std::size_t` does not become negative, add `kCapacity` and then take `%`
- If you want to add a `std::string` or a `std::function` to `MotorCommand`, stop.
  The `static_assert` in the test (`is_trivially_copyable` / `sizeof <= 4`) stops the compile

## Tests

```bash
./drill run dp22
```

There are 10 tests. Besides that it can run, they check
**whether commands run in the order they were added**, **whether undo is in reverse order**,
**whether a macro is pushed onto the history as one command**,
**whether the `std::function` version and the class version give the same result**, and
**whether the ring buffer drops the oldest ones when it exceeds its capacity**.

## References

- [22. Command](../../docs-en/patterns/22_Command.md)
- [18. Memento](../../docs-en/patterns/18_Memento.md) — when to use it versus saving the whole state
- [11. Composite](../../docs-en/patterns/11_Composite.md) — the structure of a macro command
- [cppreference: std::function](https://en.cppreference.com/w/cpp/utility/functional/function)
- [cppreference: std::move_only_function](https://en.cppreference.com/w/cpp/utility/functional/move_only_function)
