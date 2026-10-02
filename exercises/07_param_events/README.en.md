# Exercise 07: Monitor parameter changes [Intermediate]

Write the `SampleNodeWithParameters` from the official tutorial
[Monitoring for parameter changes (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Monitoring-For-Parameter-Changes-CPP.html)
exactly as it is.

## What to do

Fill in the TODOs in `src/node_with_parameters.cpp`. The specification is the same as the official tutorial.

| Item | Value |
| --- | --- |
| Node name | `node_with_parameters` |
| Parameter name | `an_int_param` |
| Type | integer |
| Default value | `0` |
| Log | `cb: Received an update to parameter "an_int_param" of type integer: "<value>"` |

The class declaration (`include/drill/node_with_parameters.hpp`) is provided. Think about what goes into the member variables
`param_subscriber_` / `cb_handle_`, and the body of the callback.

## When to use this versus exercise 06 (polling vs event notification)

In exercise 06 (`Using parameters in a class`), the timer callback calls
`get_parameter()` every time and re-reads the value. This is the "polling" style. It reads at a fixed period whether or not the parameter
has changed.

The `ParameterEventHandler` this time is the opposite. It subscribes to the `/parameter_events` topic, and the callback is called
**only when a value actually changes**. This is the "event notification" style.

A rule of thumb for which to use:

- **Polling (exercise 06)** — When you already have a loop that runs at a fixed period, and you just want to use
  the latest value along the way. It is simple to implement, and you do not need to manage the lifetime of a subscription or a handle.
- **Event notification (exercise 07)** — When you want to run something the moment a parameter changes
  (run reconfiguration code, monitor the dynamic parameters of another node, and so on).
  If nothing changes, the callback is not called, so there are no wasted reads.
  On the other hand, you take on more responsibility for keeping the `ParameterEventHandler` and `ParameterCallbackHandle` objects
  alive.

## Run it

After the tests pass, you can run it by hand, just like the official tutorial.

```bash
source install/setup.bash
ros2 run drill_07_param_events parameter_event_handler
```

In another terminal:

```bash
ros2 param set /node_with_parameters an_int_param 43
```

If you look at `/parameter_events` in yet another terminal, you can see the command above
flowing by as an actual event.

```bash
ros2 topic echo /parameter_events
```

In the terminal where `parameter_event_handler` is running, the log
`cb: Received an update to parameter "an_int_param" of type integer: "43"`
should appear.

## Common pitfalls

- **What to do with the return value of `add_parameter_callback()`.** This is the biggest
  trap this time. If you do not keep the return value (the handle) anywhere and leave it as a temporary
  object, the callback is unregistered right after you register it.
  This class already has a member you can use for that.
- The callback is delivered through `/parameter_events`. If you are not spinning
  the node, the callback is not called no matter how many times you `set_parameter()`.
- Also keep the return value for `param_subscriber_` (`std::make_shared<rclcpp::ParameterEventHandler>(this)`)
  in a member. If you receive it in a local variable, it is destroyed when the constructor
  returns.

## Differences from the official tutorial

- The official tutorial writes `main()` in the same file as the class, but here it is split into
  `src/parameter_event_handler_main.cpp` so that the tests can use the class directly.
- The official class name is `SampleNodeWithParameters`, but in this exercise it is
  `NodeWithParameters` (the node name `node_with_parameters` is the same).
- For the tests, a member `latest_value_` that the official tutorial does not have, and a public accessor
  `latest_value()`, are added. They let the tests confirm that the callback was actually called and
  received the value (the callback prints the same log as the official one, and in addition
  stores the value in `latest_value_`).

## Tests

```bash
./drill run 07
```

| Test | What it checks |
| --- | --- |
| `an_int_paramが整数型で既定値0で宣言されている` (an_int_param is declared as an integer with default value 0) | The type and the default value in `declare_parameter()` |
| `an_int_paramをsetするとlatest_valueが更新される` (setting an_int_param updates latest_value) | Whether the callback is actually called |
| `公式と同じcbログを出している` (prints the same cb log as the official one) | The format of `RCLCPP_INFO` |
| `2回目の変更でもコールバックが呼ばれる` (the callback is called on the second change too) | Whether you keep holding the handle |

## References

- Official: [Monitoring for parameter changes (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Monitoring-For-Parameter-Changes-CPP.html)
- Exercise 06: [../06_parameters/README.en.md](../06_parameters/README.en.md) (comparison with the polling style)
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
