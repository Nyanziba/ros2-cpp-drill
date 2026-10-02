# Exercise 11: Write the tests [Intermediate]

**This exercise is the reverse of the others. What you write is not the implementation but the "tests".**

Matching lecture notes: [17_Testing and debugging](../../docs-en/ros2/17_testing_and_debugging.md)

## Why this exercise is about writing tests

Chapter 17 says this.

> The function under test is just a function that does not inherit from `rclpy.node.Node` or `rclcpp::Node`.
> If you try to test a ROS node, you need `rclpy.init()`, starting the node, and spinning the executor,
> and even one test becomes heavy all at once.

In this exercise, the logic is already split out as a pure function (`limit_velocity`).
That is why the tests you write need no `rclcpp::init()`, no node startup, and no spin at all.
You can start with just two lines: `#include <gtest/gtest.h>` and `#include "drill/velocity_limiter.hpp"`.
This is the practical benefit of "splitting logic that does not depend on ROS out into an ordinary function".

## Subject: a velocity command limiter

`include/drill/velocity_limiter.hpp` declares the following function (the implementation is not public).

```cpp
double limit_velocity(double target, double previous, double max_speed, double max_delta);
```

It is a function that returns the value after applying the following two **in order**.

1. **Acceleration limit**: Keep the absolute value of the change from `previous` to `target` within `max_delta`
2. **Speed limit**: Keep the absolute value of the result of step 1 within `max_speed`

`max_speed` and `max_delta` are expected to be 0 or greater, and the specification is that a negative value is treated as 0.

## What to do

Fill in the TODOs in `test/test_exercise.cpp` and write tests for `limit_velocity`.
Only one "passing test" is already written (a normal case that does not hit any limit).
First read it, and understand how to write a test (just call `limit_velocity` directly and compare
with `EXPECT_DOUBLE_EQ`).

**However, this one test alone does not pass.** Read the next section on the grading mechanism to see what is missing.

## How grading works: just "passing" is a fail

This test is run against two implementations.

| Target | Linked implementation | Pass condition |
| --- | --- | --- |
| `test_exercise` | The correct implementation | It **passes** |
| `test_mutant` | An implementation with deliberately planted bugs | It **fails** |

In other words, it is natural that the test you wrote passes on the correct implementation, but
it also runs the same test against the **buggy implementation and checks that it really fails**.
An empty test that asserts nothing, or a test that checks only the normal case, passes `test_exercise` but
also passes `test_mutant`, so it fails the grading.

This is the other side of the claim in chapter 17. "Writing a test that passes" is easy, but
"writing a test that can detect a bug" is the real value of a test. A test that never fails
is the same as checking nothing about the specification it claims to verify.
The current TODO version of the test (only one normal case) passes `test_exercise` but also passes `test_mutant`.
So it is **still in a failing state**.

## Points you should cover

Following the TODO comments in `test/test_exercise.cpp`, test the following points.
(The expected values themselves are not written. Think about them and assert them yourself.)

- **Sign**: Does the speed limit work in the negative direction as well as the positive direction?
- **Order of application**: Are the acceleration limit and then the speed limit applied in two steps, in that order?
  (You can check it by the behavior when you try to move a lot at once.)
- **When `max_*` is 0 or negative**: Is it treated as 0, as the specification says?
- **Boundary values**: What happens at edge values, such as a change exactly equal to `max_delta`
  or a result exactly equal to `max_speed`?

Hints (the kinds of bugs. These are viewpoints, not answers):

- At least one of the planted bugs has the property that a test that checks **only one sign (direction)**
  cannot see it, but the other sign can.
- Another one is about how a **negative value passed to `max_delta` or `max_speed`** is handled.
- Every bug is made so that "adding just one viewpoint detects it".
  In other words, each viewpoint you add moves you forward for sure.

## The real thing: the node side can stay thin

The node that actually uses this function, `VelocityLimiterNode` (`src/velocity_limiter_node.cpp`), is also
provided. If you look at it, you can see that what the node does is
"subscribe, call `limit_velocity()`, and publish".
The core of the calculation (how boundary values and signs are handled) is all tested on the `limit_velocity()` side, so
a test of the node itself only needs to check "whether the wiring is correct", which is a thin one.
This is a real example of "if you split out the logic, the node side can be thin" from chapter 17.

## Run it

You can run `velocity_limiter_node` with `ros2 run`.

```bash
source install/setup.bash
ros2 run drill_11_node_test velocity_limiter_node --ros-args -p max_speed:=1.0 -p max_delta:=0.3
```

In another terminal:

```bash
ros2 topic echo /cmd_vel_limited
```

In yet another terminal:

```bash
ros2 topic pub /cmd_vel_raw std_msgs/msg/Float64 "{data: 5.0}" --once
```

Even if you send `5.0`, the `cmd_vel_limited` side should show a value limited by `max_delta` and `max_speed`
(the first time it is limited by `max_delta=0.3` to `0.3`, and if you keep sending it gradually
approaches `max_speed=1.0`).

## Tests

```bash
./drill run 11
```

(If you want to check locally with an isolated build, after `colcon build --packages-select drill_11_node_test`
you may also run `build/drill_11_node_test/test_exercise` and `build/drill_11_node_test/test_mutant`
directly.)

## References

- Lecture notes: [17_Testing and debugging](../../docs-en/ros2/17_testing_and_debugging.md)
- Official: [ROS 2 Documentation: Jazzy — Testing](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Testing/Testing-Main.html)
