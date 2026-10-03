# Exercise 06: Use parameters inside a class [Beginner]

Write the `MinimalParam` from the official tutorial
[Using parameters in a class (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Using-Parameters-In-A-Class-CPP.html)
exactly as it is.

## What to do

Fill in the TODOs in `src/minimal_param.cpp`. The specification is the same as the official tutorial.

| Item | Value |
| --- | --- |
| Node name | `minimal_param_node` |
| Parameter name | `my_parameter` |
| Type | string |
| Default value | `"world"` |
| description | `"This parameter is mine!"` |
| Period | 1000ms |
| Log | `Hello <value of my_parameter>!` |

The class declaration (`include/drill/minimal_param.hpp`) is provided. Think about what goes into the member variable
`timer_`, the parameter declaration in the constructor, and the body of the timer callback.

The key point of the official tutorial is the end of the timer callback. Even if you change
`my_parameter` with `ros2 param set`, the next time the callback is called, it is
overwritten back to `"world"`. The point of this tutorial is that a parameter should not be "read once and remembered"
but "read every time you use it".

## Run it

After the tests pass, you can run it by hand, just like the official tutorial.

```bash
source install/setup.bash
ros2 run drill_06_parameters minimal_param_node
```

In another terminal:

```bash
ros2 param list
ros2 param describe /minimal_param_node my_parameter
ros2 param get /minimal_param_node my_parameter
ros2 param set /minimal_param_node my_parameter earth
```

Right after `set` it changes to `earth`, but if you wait 1 second and run `ros2 param get` again,
it should be back to `world`. This is because the node itself writes it back with `set_parameters()` every period.

## Common pitfalls

- If you forget to pass a `ParameterDescriptor` as the third argument of `declare_parameter()`,
  the description stays empty.
- Always assign the return value of `create_wall_timer()` to `timer_`. If you receive it in a local variable,
  it is destroyed when the constructor returns, and nothing happens.
- What you pass to `set_parameters()` is a `std::vector<rclcpp::Parameter>`.
  Do not try to pass a single `rclcpp::Parameter`.
- Do not forget the `this` in `std::bind(&MinimalParam::timer_callback, this)`.

## Differences from the official tutorial

- The official tutorial writes `main()` in the same file as the class, but here it is split into
  `src/minimal_param_main.cpp` so that the tests can use the class directly.
- The official tutorial writes the timer callback as a lambda inside the constructor, but here it is
  the member function `timer_callback()` so that the tests can call it easily.
  The contents are the same as the official tutorial.

## Tests

```bash
./drill run 06
```

| Test | What it checks |
| --- | --- |
| `MyParameterIsStringWithDefaultWorld` | The type and the default value of the parameter |
| `ParameterDescriptorHasDescription` | The description passed to `declare_parameter()` |
| `TimerLogsHelloWorld` | Whether the timer is running and reads with `get_parameter()` |
| `RevertsToWorldAfterParamSet` | Whether it resets to `"world"` every period with `set_parameters()` |

## References

- Official: [Using parameters in a class (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Using-Parameters-In-A-Class-CPP.html)
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
