# Exercise 10: Write an action server [Intermediate]

Write the server side of the official tutorial
[Writing an action server and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Writing-an-Action-Server-Client/Cpp.html)
exactly as it is.

## When to use topics / services / actions

A topic is "fire and forget", and a service is "one request, one response". An action is used
when you want to ask for something that takes time (several seconds to several minutes). When you send a goal (Goal),
progress (Feedback) arrives many times along the way, and the result (Result) is returned at the end. On top of that,
you can cancel it. The Fibonacci example this time is exactly a practice ground for that.

## What to do

Fill in the TODOs in `src/fibonacci_action_server.cpp`. The specification is the same as the official tutorial.

| Item | Value |
| --- | --- |
| Node name | `fibonacci_action_server` |
| Action name | `fibonacci` |
| Type | `example_interfaces::action::Fibonacci` |
| Accepting a goal | `handle_goal` returns `ACCEPT_AND_EXECUTE` |
| Accepting a cancel | `handle_cancel` returns `ACCEPT` |
| Starting execution | `handle_accepted` starts `execute` on another thread and calls `detach()` |
| Calculation | Start `sequence` with `{0, 1}`, and from `i = 1` up to `order - 1`, `push_back` `sequence[i] + sequence[i-1]` while calling `publish_feedback` |

You can check the contents of `Fibonacci` like this.

```
$ ros2 interface show example_interfaces/action/Fibonacci
int32 order
---
int32[] sequence
---
int32[] sequence
```

From the top, separated by `---`, these are the goal (`Fibonacci::Goal`), the result (`Fibonacci::Result`), and
the progress (`Fibonacci::Feedback`). A service's `Request`/`Response` had
two parts, whereas an action has three.

The class declaration (`include/drill/fibonacci_action_server.hpp`) is provided. Think about filling in the four
callbacks (`handle_goal` / `handle_cancel` / `handle_accepted` / `execute`), and
storing the return value of `rclcpp_action::create_server` in `action_server_` in the constructor.

### Differences from the official tutorial

There are only the following two differences from the official tutorial. The logic (the contents of the callbacks,
the wording of the logs, and the contents of the result) is exactly the same as the official one.

- `main()` is split into a separate file (`src/fibonacci_action_server_main.cpp`)
  (so that the tests can create the class directly and check it).
- The loop period of `execute()` is changed from the official 1 second (`rclcpp::Rate loop_rate(1)`) to
  **20ms**. This is because with the official 1 second, tests with a large `order`
  would be too slow.

## Run it

After the tests pass, you can run it by hand, just like the official tutorial.

```bash
source install/setup.bash
ros2 run drill_10_action_server fibonacci_action_server
```

In another terminal:

```bash
ros2 action list                                                          # can you see /fibonacci?
ros2 action info /fibonacci -t
ros2 action send_goal /fibonacci example_interfaces/action/Fibonacci "{order: 5}" --feedback
```

With `--feedback`, the `Publish feedback` log appears in the server terminal, and
the progress (`sequence`) appears many times in the client terminal.
If a result such as `Result: sequence=[0, 1, 1, 2, 3, 5]` appears at the end, it works.

If you stop the sending with `Ctrl-C`, it becomes a cancel request, and the server prints the logs
`Received request to cancel goal` / `Goal canceled`.

## Common pitfalls

- Always assign the return value of `create_server()` to `action_server_`. If you receive it in a local variable,
  it is destroyed when the constructor returns, and the action disappears
  (the same trap as the Publisher/Timer in 01 and the Service in 03).
- It is important that `handle_accepted` "returns quickly". If you call `execute` directly here,
  the Executor is blocked for a long time and cannot process any other callbacks (such as cancel requests).
  As in the official tutorial, hand it off to another thread with `std::thread{...}.detach()`.
- Put the `is_canceling()` check in `execute` **before** `publish_feedback`
  (the same order as the official tutorial). If you put it after, one extra feedback is sent right after
  the cancel.
- For the index in `sequence.push_back(sequence[i] + sequence[i - 1])`, use the loop variable `i`
  as it is. `sequence` starts from `{0, 1}`, so when `i = 1`,
  you calculate `sequence[1] + sequence[0]`.
- `goal->order` is an `int32_t`. The loop condition is `i < goal->order`, not
  `<=`.

## Tests

```bash
./drill run 10
```

| Test | What it checks |
| --- | --- |
| `fibonacciアクションサーバを公開している` (provides the fibonacci action server) | Whether `create_server` is stored in `action_server_` |
| `order5の目標を送るとフィボナッチ数列を返す` (sending a goal with order 5 returns the Fibonacci sequence) | The calculation logic and `succeed(result)` |
| `実行中にfeedbackが1回以上届く` (feedback arrives at least once during execution) | Whether `publish_feedback` is called |
| `キャンセル要求を受理する` (accepts the cancel request) | `handle_cancel` and `is_canceling()` / `canceled(result)` |

## References

- Official: [Writing an action server and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Writing-an-Action-Server-Client/Cpp.html)
- Official: [Creating an action](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Creating-an-Action.html)
- Official example implementation (action_tutorials_cpp in ros2/demos): <https://github.com/ros2/demos/tree/rolling/action_tutorials/action_tutorials_cpp>
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
