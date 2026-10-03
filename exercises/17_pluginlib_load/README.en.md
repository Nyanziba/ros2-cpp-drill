# Exercise 17: Load a plugin with ClassLoader (pluginlib) [Advanced]

This is the "loading side" of the official tutorial
[Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html).
The 3 filters made in exercise 16 (`ClampFilter` / `RateLimitFilter` / `DeadbandFilter`) are provided in finished form.
You write **`FilterHost`, a node that chooses and loads one of them by a parameter**.

## What to do

Fill in the TODOs in `include/drill/filter_host.hpp` and `src/filter_host.cpp`.

| Item | Value |
| --- | --- |
| Class name | `FilterHost` (an `rclcpp::Node`; the name is `filter_host`) |
| Parameter | `filter_type` (a string; the default is empty). A pluginlib name (for example `drill/ClampFilter`) |
| Name passed to the plugin | `kFilterInstanceName` (`"filter"`). Parameters look like `filter.max_linear` |
| ClassLoader | `pluginlib::ClassLoader<drill::VelocityFilter>("drill_17_pluginlib_load", "drill::VelocityFilter")` |

The public methods are declared. You declare the members (the ClassLoader and the chosen filter) in the header yourself.

1. **Declare the members.** The `ClassLoader` and the chosen filter (`std::shared_ptr<drill::VelocityFilter>`).
   **The order of the declarations matters** (see "Common pitfalls" below).
2. **`configure()`**: read `filter_type`, create the plugin with `createSharedInstance`,
   call `initialize(shared_from_this(), kFilterInstanceName)`, and store it in the member.
3. **When the name is not found** (`pluginlib::PluginlibException`), catch it, put the name that could not be loaded
   and `e.what()` in an `RCLCPP_ERROR`, and **leave the filter empty (pass through).** Do not bring the node down.
4. **`apply(command, dt_seconds)`**: return the result of passing it through the filter if there is one; if it is empty, return `command` as it is.
5. **`available_filter_names()`**: return the list of usable names (`getDeclaredClasses()`).

`configure()` exists separately because `initialize` receives the node as an `rclcpp::Node::SharedPtr`.
`shared_from_this()` cannot be used until `std::make_shared` has finished and the object is inside a `shared_ptr`
(calling it inside the constructor throws an exception). Only the `ClassLoader` is created in the constructor's initializer list.

## Try it

```bash
./drill run 17
source install/setup.bash
ros2 run drill_17_pluginlib_load filter_host_node --ros-args -p filter_type:=drill/ClampFilter
ros2 run drill_17_pluginlib_load filter_host_node --ros-args -p filter_type:=drill/Typo
```

The second one is a name that does not exist. If it does not crash and shows an error with the name that could not be loaded
and the list of usable names, it works (stop it with Ctrl-C).

## Common pitfalls

- **Declare the `ClassLoader` before the filter.** Members are destroyed in reverse order of declaration.
  If you declare the filter first, the `ClassLoader` is destroyed while the filter is still alive.
  class_loader prints the warning `SEVERE WARNING!!! Attempting to unload library while objects created by this loader exist in the heap!`
  to standard error, and the behavior can be undefined. The test captures standard error and checks that this warning does not appear.
- What `createSharedInstance` throws is `pluginlib::PluginlibException`. Catching `std::exception` also works,
  but catch `PluginlibException` so that you catch only loading failures.
- When it fails, **leave the member filter empty.** If you keep a half-made filter (one whose `initialize` failed),
  it does not pass through and runs in a half-working state.
- If you write `initialize(shared_from_this(), ...)` inside the constructor, it crashes with `bad_weak_ptr`.

## Tests

```bash
./drill run 17
```

| Test | What it checks |
| --- | --- |
| `SelectsFilterByFilterTypeParameter` | Whether one of the 3 is chosen by the name in `filter_type`, configured by `filter.<item>`, and keeps its state |
| `FallsBackToPassThroughWhenNameIsUnknown` | Whether it does not crash on a name that does not exist, passes through, and puts the name in the log |
| `PassesThroughWhenFilterTypeIsNotGiven` | Whether it passes through without crashing even when `filter_type` is not given |
| `ListsAvailableFilterNames` | Whether `available_filter_names()` contains the 3 names |
| `HostWithLiveFilterIsDestroyedWithoutClassLoaderWarning` | Whether destroying it while it holds a filter prints no class_loader warning (whether you declared the `ClassLoader` first) |

## References

- Official: [Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
- Lecture: [ROS 2 Lecture 15b: pluginlib](../../docs-en/ros2/15b_pluginlib.md)
