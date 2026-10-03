# Exercise 16: Create a plugin and export it (pluginlib) [Advanced]

This exercise does the "plugin author" side of the official tutorial
[Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html),
using filters applied to a velocity command (`cmd_vel`) as the subject.
The reading side (`ClassLoader`) is covered in exercises 17 and 18.

The components of exercise 15 were a mechanism for swapping **nodes**. pluginlib is a mechanism for
**swapping the parts inside a node**. You decide a base class (`drill::VelocityFilter`) in advance,
and load classes that inherit it at run time, chosen by name (a string).

## What to do

Of the 3 filters (plugins), you write the body of `ClampFilter` and the 3 "exports".
The bodies of `RateLimitFilter` and `DeadbandFilter` are provided.

| Class | pluginlib name | Parameters (defaults) | Behavior |
| --- | --- | --- | --- |
| `drill::ClampFilter` | `drill/ClampFilter` | `<name>.max_linear` (1.0), `<name>.max_angular` (2.0) | Cuts the absolute value at the limit |
| `drill::RateLimitFilter` | `drill/RateLimitFilter` | `<name>.max_linear_acceleration` (0.5), `<name>.max_angular_acceleration` (1.0) | Limits the change from the previous output to acceleration x dt |
| `drill::DeadbandFilter` | `drill/DeadbandFilter` | `<name>.linear_threshold` (0.05), `<name>.angular_threshold` (0.1) | Zero if the absolute value is below the threshold |

The base class (`include/drill/velocity_filter.hpp`) is provided. It handles only `linear.x` and `angular.z`.
A plugin is **created with a constructor that takes no arguments**, so it receives its settings in `initialize(node, name)`
(`name` is the YAML namespace; parameters are `<name>.<item>`).

There are 4 TODOs.

1. **`ClampFilter::filter` in `src/filters.cpp`.** Cut `linear.x` to `[-max_linear_, +max_linear_]` and
   `angular.z` to `[-max_angular_, +max_angular_]`, and return it. Cut the negative direction too.
2. **Export the 3 classes at the end of `src/filters.cpp`.**
   ```cpp
   #include <pluginlib/class_list_macros.hpp>
   PLUGINLIB_EXPORT_CLASS(drill::ClampFilter, drill::VelocityFilter)
   ```
   for each of the 3 classes. When the library is dlopen'ed, this registers the part that creates the class.
3. **Declare the 3 classes with `<class>` in `plugins.xml`.** Write `name` (the name the caller uses),
   `type` (the C++ class name) and `base_class_type` (the base class).
4. **Register `plugins.xml` in `cmake/export_plugins.cmake`.**
   Call `pluginlib_export_plugin_description_file(<package that has the base class> <path of the XML>)`.
   The base class is inside this package, so the first argument is yourself (`${PROJECT_NAME}`).

> Only the CMake registration is split out into `cmake/export_plugins.cmake` instead of `CMakeLists.txt`.
> If a `CMakeLists.txt` of the exercise were also placed in `templates/` and `solutions/`, colcon would mistake
> them for separate packages. `CMakeLists.txt` only `include()`s this file, and you do not edit it.

## Try it

```bash
./drill run 16
```

When it passes, look at the result of your export.

```bash
source install/setup.bash
cat install/drill_16_pluginlib_create/share/drill_16_pluginlib_create/plugins.xml
ls install/drill_16_pluginlib_create/lib/
```

`plugins.xml` and the shared library (`libdrill_16_pluginlib_create_filters.so`) are installed, and
`<library path=...>` points to its name (without `lib` and `.so`).
The test does not link against this library. `pluginlib::ClassLoader` finds and loads it at run time,
relying on `plugins.xml`.

## Common pitfalls

- If **any one** of the 3 exports (`PLUGINLIB_EXPORT_CLASS`, `plugins.xml`, `pluginlib_export_plugin_description_file`)
  is missing, the build still passes. The failure comes at run time (when `ClassLoader` reads it).
  The failure messages of the tests guide you to where to look.
- The `name` (`drill/ClampFilter`) and the `type` (`drill::ClampFilter`) in `plugins.xml` are different things.
  The caller chooses by `name`, and `type` is what pluginlib uses to match against the C++ class.
- `<library path=...>` is the name of the library made with `add_library`. If you rename it, fix the XML too.
- The plugin library is **`SHARED`**. A static library cannot be something that is loaded at run time.
- If you write the limit of `ClampFilter` as a fixed value (`1.0`) in `filter()`, you get a filter that cannot be changed with parameters
  (use the `max_linear_` that `initialize` read and stored).

## Tests

```bash
./drill run 16
```

| Test | What it checks |
| --- | --- |
| `ThreePluginsAreDeclared` | Whether the 3 names reach `ClassLoader` from `plugins.xml` (both the `<class>` and the CMake registration) |
| `ThreePluginsCanBeCreated` | Whether all 3 can actually be created (`PLUGINLIB_EXPORT_CLASS`, and the `path` of the library) |
| `ClampFilterCutsAbsoluteValueAtLimits` | Whether it cuts both directions at the default limits (1.0 / 2.0), and lets values inside the limits pass |
| `ClampFilterUsesParametersUnderInstanceName` | Whether it uses `limit.max_linear` for the name given (`limit`) |
| `RateLimitAndDeadbandWorkThroughClassLoader` | Whether the 2 provided ones work through `ClassLoader` |

## References

- Official: [Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
- Lecture: [ROS 2 Lecture 15b: pluginlib](../../docs-en/ros2/15b_pluginlib.md)
- Next exercises: 17 (load with `ClassLoader`), 18 (build a chain of plugins from YAML)
