# Exercise 18: Build a chain of plugins from YAML parameters (pluginlib) [Advanced]

Exercise 17 was "choose one by a parameter". In this exercise you write **`FilterPipeline`, a node that takes a list of names
from YAML and lines up the plugins in that order to pass commands through them**. It has the same shape as nav2 building its
controller / planner / costmap layers from a list of names in YAML. The 3 filters (the ones made in exercise 16) are provided in finished form.

```yaml
filter_host:
  ros__parameters:
    filters: ["deadband", "clamp", "rate_limit"]   # pass through in this order
    deadband:
      plugin: "drill/DeadbandFilter"               # <name>.plugin is the pluginlib name
      linear_threshold: 0.05                       # <name>.<item> is declared and read by the plugin itself
    clamp:
      plugin: "drill/ClampFilter"
      max_linear: 0.5
    rate_limit:
      plugin: "drill/RateLimitFilter"
      max_linear_acceleration: 1.0
```

## What to do

Fill in the TODOs in `include/drill/filter_pipeline.hpp` and `src/filter_pipeline.cpp`.

| Item | Value |
| --- | --- |
| Class name | `FilterPipeline` (an `rclcpp::Node`; the name is `filter_host`, the same as the top-level key of the YAML) |
| Order | The parameter `filters` (an array of strings; the default is empty) |
| Plugin name of each stage | `<name>.plugin` (a string) |
| Name passed to the plugin | The stage name `name`. The plugin declares `<name>.<item>` |
| ClassLoader | `pluginlib::ClassLoader<drill::VelocityFilter>("drill_18_pluginlib_yaml", "drill::VelocityFilter")` |
| Subscribe / output | Subscribe to `cmd_vel_in` -> `apply` -> publish to `cmd_vel_out` (`geometry_msgs/msg/Twist`, QoS depth 10) |
| `dt_seconds` | The subscriber passes `kCommandPeriodSeconds` (0.05) |

1. **Declare the members.** The `ClassLoader`, the list of filters (`std::vector<std::shared_ptr<drill::VelocityFilter>>`),
   the subscription, and the publisher. **Declare the `ClassLoader` before the list of filters.**
2. **`configure()`**: read `filters`, read `<name>.plugin` for each `name`, create the plugin,
   call `initialize(shared_from_this(), name)`, and put it into the list **in the order you read them**. Create the subscription and the publisher here too.
3. **For an unknown plugin name** (`pluginlib::PluginlibException`), catch it with a `try / catch` for each stage,
   put in an `RCLCPP_ERROR` "which stage (`name`), which plugin name, and why (`e.what()`)" could not be loaded, and
   **skip only that stage.** Keep building the remaining stages, and do not bring the node down.
4. **`apply(command, dt_seconds)`**: return the result of passing through the filters in list order (`command` as it is if the list is empty).
5. **`filter_count()`**: return the number of stages that were built (stages that could not be loaded are not counted).

## Try it

Start it with the bundled YAML (`config/filters.yaml`) and send commands from another terminal.

```bash
./drill run 18
source install/setup.bash
ros2 run drill_18_pluginlib_yaml filter_pipeline_node --ros-args \
  --params-file install/drill_18_pluginlib_yaml/share/drill_18_pluginlib_yaml/config/filters.yaml
```

```bash
ros2 topic echo /cmd_vel_out                                   # another terminal
ros2 topic pub -r 10 /cmd_vel_in geometry_msgs/msg/Twist "{linear: {x: 1.0}}"   # another terminal
```

Swap the order of `filters`, or mistype a `plugin`, and see how the output changes.
Instead of `--params-file`, you can pass the YAML in the same way with `parameters=[...]` in a launch file.

## Common pitfalls

- **The order changes the result.** Cutting at a limit and then zeroing with a threshold gives a different result from
  passing through the threshold and then cutting (the test compares these 2 ways). `push_back` in the order of `filters`.
- The top-level key of the YAML (`filter_host`) must **match the node name.**
  If it does not, the parameters are not read and `filters` stays empty.
- Read `<name>.plugin` **after declaring it.** A parameter that is not declared cannot be read even if it is written in the YAML.
  `<name>.max_linear` and the like are declared by the plugin inside `initialize` (the host does not declare them).
- Do not stop everything because one stage failed. And do not forget to **leave the stage that could not be read out of the list.**
- Keep the subscription and the publisher in members. As local variables, they disappear when you leave `configure()`.
- Declare the `ClassLoader` before the list of filters (if it is the other way around, class_loader prints a warning on destruction and the behavior can be undefined, the same as exercise 17).

## Tests

```bash
./drill run 18
```

The YAML files are in `test/` and are loaded into the node with `--ros-args --params-file`.

| Test | What it checks |
| --- | --- |
| `AppliesFiltersInListedOrder` | Whether it passes through in the order of `filters` (whether the result changes between 2 YAML files with the same 2 stages swapped) |
| `PassesEachStageItsOwnParameters` | Whether it chooses by the `plugin` of each stage and each plugin gets its own `<name>.<item>` (items not given stay at their defaults) |
| `SkipsStageWithUnknownPluginAndKeepsTheRest` | Whether it does not crash on an unknown plugin name, logs the stage name and the plugin name, skips only that stage, and the rest keep working |
| `PassesThroughWhenFiltersIsEmpty` | Whether there are 0 stages and it passes through when there is no `filters` |
| `PublishesFilteredCommandToCmdVelOut` | Whether it subscribes to `cmd_vel_in`, passes through the filters, and outputs to `cmd_vel_out` |
| `PipelineWithLiveFiltersIsDestroyedWithoutClassLoaderWarning` | Whether destroying it while it holds filters prints no class_loader warning (whether you declared the `ClassLoader` first) |

## References

- Official: [Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
- Lecture: [ROS 2 Lecture 15b: pluginlib](../../docs-en/ros2/15b_pluginlib.md)
- Previous exercises: 16 (create a plugin), 17 (load with `ClassLoader`)
