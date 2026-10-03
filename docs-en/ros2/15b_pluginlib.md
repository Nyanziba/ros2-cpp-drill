# ROS 2 Lecture 15b: Swapping Parts from Configuration with pluginlib

## Introduction

After this article, you will be able to build the following with `pluginlib` yourself: you decide a base class, export implementations as plugins, and load them by the names you write in YAML. Standard ROS 2 packages (such as nav2 and ros2_control) change their behavior just by a name after `plugin:` in a configuration file. This mechanism is what makes that possible.

In "Going further" of [04_nodes](04_nodes.md), we saw that a component is a way to put *nodes* into the same process. pluginlib works one level inside that: it swaps the parts inside a node. You can read it as a sequel to [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md), which covers parameters and YAML.

The prerequisite is that you have finished [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md) and [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md), and that you can build a C++ package (ament_cmake). We also use base classes and inheritance (`virtual`).

## Goals

- Explain what pluginlib does and when to use it (and when not to)
- Export a plugin with a base class, a plugin class, `PLUGINLIB_EXPORT_CLASS`, `plugins.xml`, and the CMake and package.xml settings
- Create a plugin from its name with `pluginlib::ClassLoader`, and handle failures with `PluginlibException`
- Build a chain of plugins from a `filters` list in YAML and per-plugin namespaced parameters

## Main text

### 1. What is pluginlib?

pluginlib is a library that **creates an object of a class at run time from the class name (a string)**. It has four parts.

- **Base class**: the contract (interface) that the swappable parts follow. The user of the parts (the host) decides it
- **Plugin**: an implementation that inherits the base class. It goes into a shared library (`.so`) and is registered with `PLUGINLIB_EXPORT_CLASS`
- **`plugins.xml`**: a table that says "this name is this class in this library"
- **`ClassLoader`**: a tool inside the host that finds a plugin by name and creates it

The host code knows nothing about the plugin implementations (it needs neither their headers nor linking). So **you can swap implementations just by changing a string in the configuration, without editing or rebuilding the host code**. You can also add implementations written in another package later.

When to use it:

- Use it when: you want to choose the implementation by configuration, the implementations keep growing, or the people who write the host and the implementations are different (different packages or teams)
- Do not use it when: there is only one implementation, or only the code writer swaps it and rebuilding is not a problem. A plain `virtual` class or a function is enough then, and pluginlib only adds machinery

#### Difference from components

The components in [04_nodes](04_nodes.md) (exercise 15) also load a shared library at run time. The difference is what they swap.

| | component (`rclcpp_components`) | pluginlib |
| --- | --- | --- |
| What it swaps | the **node** itself | a **part inside a node** |
| Base class | fixed to `rclcpp::Node` | you decide it (`VelocityFilter` in this chapter) |
| Registration | `RCLCPP_COMPONENTS_REGISTER_NODE` | `PLUGINLIB_EXPORT_CLASS` + `plugins.xml` |
| Loaded by | `component_container` (provided by ROS) | the `ClassLoader` in your own code |

A component chooses "which nodes go into the same process". pluginlib chooses "which parts a node uses".

### 2. Examples in practice

Major ROS 2 packages all swap parts with pluginlib. Here is "what is the base class, and where do you choose it".

| User | Base class (the swappable part) | Where you choose it |
| --- | --- | --- |
| nav2 planner server | `nav2_core::GlobalPlanner` | `planner_plugins` in YAML is a list of names, and each name has `plugin:` with the class name |
| nav2 controller server | `nav2_core::Controller` | `controller_plugins` in YAML is a list of names, and each name has `plugin:` with the class name |
| nav2 costmap | `nav2_costmap_2d::Layer` | `plugins` in YAML is a list of layer names, and each layer has `plugin:` with the class name |
| ros2_control | `ControllerInterface` for controllers, `SystemInterface` and others for hardware | `type:` under each controller name in YAML for controllers. `<plugin>` inside `<ros2_control>` in the URDF for hardware |
| rviz2 | `rviz_common::Display` | Not YAML: you choose it with "Add" in the GUI. The settings are saved in a `.rviz` file |
| image_transport | `image_transport::PublisherPlugin` / `SubscriberPlugin` | The `image_transport` parameter on the subscriber side, set to a transport name such as `raw` or `compressed` |

For example, the nav2 planner server is written like this (an example from the nav2 documentation).

```yaml
planner_server:
  ros__parameters:
    planner_plugins: ["GridBased"]
    GridBased:
      plugin: "nav2_navfn_planner::NavfnPlanner"
```

This form, **a list of names, and a namespace for each name that holds `plugin:` and that plugin's settings**, is the standard way to configure plugins in ROS 2. The pipeline we build in the second half of this chapter uses the same form.

The plugin "name" is the string you write in `name` in `plugins.xml`. nav2 uses C++ class names such as `nav2_navfn_planner::NavfnPlanner`. ros2_control uses the `package_name/ClassName` form such as `joint_state_broadcaster/JointStateBroadcaster`. Both work as long as they match `name` in `plugins.xml`. This chapter uses the second form (`drill/ClampFilter`).

### 3. How to build one

The subject is a filter applied to a velocity command (`geometry_msgs/msg/Twist`). It handles only `linear.x` and `angular.z`. We make three filters.

| Class | Name | Behavior |
| --- | --- | --- |
| `drill::ClampFilter` | `drill/ClampFilter` | Clamps the absolute value to an upper limit |
| `drill::RateLimitFilter` | `drill/RateLimitFilter` | Limits the change from the previous output to acceleration x elapsed time |
| `drill::DeadbandFilter` | `drill/DeadbandFilter` | Sets the value to 0 when its absolute value is below a threshold |

#### The base class: receive settings in `initialize()`

```cpp
class VelocityFilter
{
public:
  virtual ~VelocityFilter() = default;
  virtual void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) = 0;
  virtual geometry_msgs::msg::Twist filter(
    const geometry_msgs::msg::Twist & command, double dt_seconds) = 0;
protected:
  VelocityFilter() = default;
};
```

The important point is that **the constructor takes no arguments**. pluginlib knows only a name, so it creates the object with a no-argument constructor. You cannot pass settings such as a threshold through constructor arguments. Instead, the settings arrive in `initialize()`, which is called after creation. `node` is passed so that the plugin can declare and read parameters, and `name` is "this plugin's namespace in YAML". The plugin declares parameters under its own namespace, such as `<name>.max_linear`.

Making the destructor `virtual` is also part of the contract, because the object is deleted through a base class pointer.

#### The plugin class and `PLUGINLIB_EXPORT_CLASS`

A plugin inherits the base class and implements `initialize()` and `filter()`. At the end of the source file, you register it with `PLUGINLIB_EXPORT_CLASS(implementation class, base class)`.

```cpp
#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(drill::ClampFilter, drill::VelocityFilter)
```

When the shared library is loaded, this macro registers that "`ClampFilter` can be created as a `VelocityFilter`". The full classes are in "Try it yourself".

#### plugins.xml

Write the table of names, classes, and libraries in `plugins.xml` at the top of the package.

```xml
<library path="velocity_filters">
  <class name="drill/ClampFilter" type="drill::ClampFilter"
         base_class_type="drill::VelocityFilter">
    <description>Clamps the absolute value of the speeds.</description>
  </class>
</library>
```

- `library path`: the library name, without the `lib` prefix and the `.so` extension (the name in CMake `add_library(velocity_filters ...)`)
- `class name`: **the name you use in the host and in YAML**
- `type`: the implementation class name, the same as the first argument of `PLUGINLIB_EXPORT_CLASS`
- `base_class_type`: the base class name, the same as the second argument of `PLUGINLIB_EXPORT_CLASS`

#### CMake and package.xml

In CMake, build the library and export `plugins.xml` with `pluginlib_export_plugin_description_file`. Write the dependencies with `target_link_libraries`.

```cmake
find_package(pluginlib REQUIRED)

add_library(velocity_filters SHARED src/velocity_filters.cpp)
target_link_libraries(velocity_filters PUBLIC
  rclcpp::rclcpp ${geometry_msgs_TARGETS} pluginlib::pluginlib)

pluginlib_export_plugin_description_file(velocity_filter_demo plugins.xml)
```

- Do not use `ament_target_dependencies`. It was removed in Lyrical. `target_link_libraries` also works in Jazzy, so this is the form that works in both
- The first argument of `pluginlib_export_plugin_description_file` is **the package that has the base class**. In this chapter the base class and the implementations are in the same package, so it is its own name. When the base class is in another package and only the implementations are in this one, pass the name of the package with the base class
- If you forget this call, `plugins.xml` is not registered where it can be found, and the host cannot find the names (we will cause this in "Try it yourself")

Add `<depend>pluginlib</depend>` to `package.xml`. If the host and the implementations are in different packages, the implementation package also needs a `<depend>` on the package with the base class.

### 4. How to use it (load with ClassLoader)

The host uses only the base class header and `pluginlib/class_loader.hpp`. It does not need the implementation headers.

```cpp
#include "pluginlib/class_loader.hpp"

pluginlib::ClassLoader<drill::VelocityFilter> loader("velocity_filter_demo", "drill::VelocityFilter");
auto filter = loader.createUniqueInstance("drill/ClampFilter");
filter->initialize(node, "clamp");
```

- Give the `ClassLoader` constructor **the name of the package that has the base class** and **the name of the base class**
- `createSharedInstance("name")` returns a `std::shared_ptr`, and `createUniqueInstance("name")` returns a `pluginlib::UniquePtr` (a relative of `std::unique_ptr`). If there is one owner, `createUniqueInstance` is the natural choice
- `getDeclaredClasses()` returns the list of registered names. You can use it in an error message for a wrong name, or to show a list
- Failures such as "the name is not found" or "the library cannot be opened" are reported as **`pluginlib::PluginlibException`**. Its `what()` message contains the name that was searched for and the registered names. **It is the host's job to catch it and decide: stop with a clear message, or skip the plugin.** You can also check first with `isClassAvailable("name")`

#### Keep the ClassLoader alive longer than the instances

The code of an object created by `createUniqueInstance` or `createSharedInstance` lives in the shared library. If the `ClassLoader` is destroyed first, it tries to unload the library, and the code of the objects that are still alive would disappear. So **the `ClassLoader` must be destroyed after the instances it created**.

The members of a C++ class are **destroyed in reverse order of declaration**. So declare the `ClassLoader` member **before** the members that hold instances.

```cpp
private:
  pluginlib::ClassLoader<VelocityFilter> loader_;                   // declared first = destroyed last
  std::vector<pluginlib::UniquePtr<VelocityFilter>> filters_;       // declared later = destroyed first
```

We measure what happens with the opposite order in "Try it yourself".

### 5. How to configure with YAML

We use the same form as nav2: **a list of the plugin names to use** (`filters`), and **a namespace for each name** that holds the plugin class name (`plugin`) and the settings.

```yaml
filter_host:
  ros__parameters:
    filters: ["deadband", "clamp", "rate_limit"]   # applied in this order
    deadband:
      plugin: "drill/DeadbandFilter"
      linear_threshold: 0.05
    clamp:
      plugin: "drill/ClampFilter"
      max_linear: 0.5
    rate_limit:
      plugin: "drill/RateLimitFilter"
      max_linear_acceleration: 1.0
```

The host declares and reads `filters` and each `<name>.plugin`, and calls `initialize(node, name)`. Each plugin declares its own `<name>.<item>`. For example, the `clamp` plugin declares `clamp.max_linear` and `clamp.max_angular`.

#### Good practice

- **Give each plugin its own namespace.** Put items under the name, like `deadband.linear_threshold`. If you put `max_linear: 0.5` at the top level, it collides with the same item of another plugin. The name is **the name of the instance, not the class name**, so you can use the same class twice with different settings (for example `clamp_slow` and `clamp_fast`). Do not repeat a name, though
- **Give every item a default value.** The plugin has a default through `declare_parameter`, and the YAML holds only the items you want to change. The `deadband` in the example above does not write `angular_threshold`, but it works with the default `0.1`. The YAML gets shorter, and "it does not start because I forgot one item" becomes rarer
- **Decide how to treat an unknown name.** When the name in `plugin` is not registered, decide as a design choice of the host whether to "stop with a clear message" or "skip that plugin and continue". For something that must be active, such as a velocity safety filter, **stopping** is safer (if you skip it, the robot runs without the filter). For a decorative part, skipping is fine. Either way, do not stay silent: print a message
- **Write a list whose order matters as a YAML array.** When the order changes the result, as with `filters` in this chapter, use the order of an array. Do not rely on the order of the per-name blocks (parameters are a dictionary of names, so the order you wrote is not guaranteed)

#### Practices to avoid

- Making something a plugin when you do not need to swap it (for example, there is only one implementation)
- Writing a class name directly in C++ code. Then you cannot swap it by configuration
- Packing settings into one string (such as `"clamp:0.5"`). If you write them as typed parameters, type mismatches are also checked
- Making every item required, with no defaults. The YAML gets long, and forgetting one item stops the start-up
- Silently skipping an unknown name. You will not notice a typo
- Not matching the numeric type. If you drop the decimal point, as in `max_linear: 1`, it becomes an integer and does not fit a parameter declared as `double` (we measure this in "Try it yourself")

## Try it yourself

In `~/ros2_ws/src/`, we make a small package `velocity_filter_demo` that holds the base class, the three filters, and a host that builds a pipeline from YAML. There are seven files. First create an empty package, then put the contents of the files below.

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake --license Apache-2.0 velocity_filter_demo
mkdir -p velocity_filter_demo/include/drill velocity_filter_demo/config
```

Replace the `package.xml` and `CMakeLists.txt` made by `ros2 pkg create` with the following.

```xml
<?xml version="1.0"?>
<!-- ~/ros2_ws/src/velocity_filter_demo/package.xml -->
<package format="3">
  <name>velocity_filter_demo</name>
  <version>0.1.0</version>
  <description>pluginlib demo: velocity filters</description>
  <maintainer email="you@example.com">you</maintainer>
  <license>Apache-2.0</license>

  <buildtool_depend>ament_cmake</buildtool_depend>

  <depend>rclcpp</depend>
  <depend>geometry_msgs</depend>
  <depend>pluginlib</depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
```

```cmake
# ~/ros2_ws/src/velocity_filter_demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.8)
project(velocity_filter_demo)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(pluginlib REQUIRED)

# The plugin implementations. The host does not link to this library (it loads it at run time).
add_library(velocity_filters SHARED src/velocity_filters.cpp)
target_include_directories(velocity_filters PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(velocity_filters PUBLIC
  rclcpp::rclcpp ${geometry_msgs_TARGETS} pluginlib::pluginlib)

# The host. It uses only the base class header and pluginlib.
add_executable(filter_host src/filter_host.cpp)
target_include_directories(filter_host PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(filter_host
  rclcpp::rclcpp ${geometry_msgs_TARGETS} pluginlib::pluginlib)

install(TARGETS velocity_filters LIBRARY DESTINATION lib)
install(TARGETS filter_host DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})

# Export plugins.xml so that it can be found at run time.
pluginlib_export_plugin_description_file(velocity_filter_demo plugins.xml)

ament_package()
```

The base class. The host and all plugins share only this header.

```cpp
// ~/ros2_ws/src/velocity_filter_demo/include/drill/velocity_filter.hpp
#ifndef DRILL_VELOCITY_FILTER_HPP_
#define DRILL_VELOCITY_FILTER_HPP_

#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"

namespace drill
{

class VelocityFilter
{
public:
  virtual ~VelocityFilter() = default;

  // pluginlib creates objects with a no-argument constructor, so settings arrive here.
  // name is the YAML namespace (e.g. "clamp"). Declare parameters as "<name>.<item>".
  virtual void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) = 0;

  // dt_seconds is the time since the previous command.
  virtual geometry_msgs::msg::Twist filter(
    const geometry_msgs::msg::Twist & command, double dt_seconds) = 0;

protected:
  VelocityFilter() = default;
};

}  // namespace drill

#endif  // DRILL_VELOCITY_FILTER_HPP_
```

The three filters and their export.

```cpp
// ~/ros2_ws/src/velocity_filter_demo/src/velocity_filters.cpp
#include <algorithm>
#include <cmath>
#include <string>

#include "drill/velocity_filter.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace drill
{

// Clamps the absolute value to an upper limit.
class ClampFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    max_linear_ = node->declare_parameter<double>(name + ".max_linear", 1.0);
    max_angular_ = node->declare_parameter<double>(name + ".max_angular", 2.0);
  }

  geometry_msgs::msg::Twist filter(const geometry_msgs::msg::Twist & command, double) override
  {
    geometry_msgs::msg::Twist result = command;
    result.linear.x = std::clamp(command.linear.x, -max_linear_, max_linear_);
    result.angular.z = std::clamp(command.angular.z, -max_angular_, max_angular_);
    return result;
  }

private:
  double max_linear_ = 1.0;
  double max_angular_ = 2.0;
};

// Limits the change from the previous output to acceleration x dt.
class RateLimitFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    max_linear_acceleration_ = node->declare_parameter<double>(name + ".max_linear_acceleration", 0.5);
    max_angular_acceleration_ = node->declare_parameter<double>(name + ".max_angular_acceleration", 1.0);
  }

  geometry_msgs::msg::Twist filter(const geometry_msgs::msg::Twist & command, double dt_seconds) override
  {
    geometry_msgs::msg::Twist result = command;
    result.linear.x = approach(previous_.linear.x, command.linear.x, max_linear_acceleration_ * dt_seconds);
    result.angular.z = approach(previous_.angular.z, command.angular.z, max_angular_acceleration_ * dt_seconds);
    previous_ = result;
    return result;
  }

private:
  static double approach(double previous, double target, double max_step)
  {
    return previous + std::clamp(target - previous, -max_step, max_step);
  }

  double max_linear_acceleration_ = 0.5;
  double max_angular_acceleration_ = 1.0;
  geometry_msgs::msg::Twist previous_;  // The first call is treated as a change from 0
};

// Sets the value to 0 when its absolute value is below the threshold.
class DeadbandFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    linear_threshold_ = node->declare_parameter<double>(name + ".linear_threshold", 0.05);
    angular_threshold_ = node->declare_parameter<double>(name + ".angular_threshold", 0.1);
  }

  geometry_msgs::msg::Twist filter(const geometry_msgs::msg::Twist & command, double) override
  {
    geometry_msgs::msg::Twist result = command;
    if (std::abs(command.linear.x) < linear_threshold_) {
      result.linear.x = 0.0;
    }
    if (std::abs(command.angular.z) < angular_threshold_) {
      result.angular.z = 0.0;
    }
    return result;
  }

private:
  double linear_threshold_ = 0.05;
  double angular_threshold_ = 0.1;
};

}  // namespace drill

// The first argument is the implementation, the second is the base class.
// Use the same names as type / base_class_type in plugins.xml.
PLUGINLIB_EXPORT_CLASS(drill::ClampFilter, drill::VelocityFilter)
PLUGINLIB_EXPORT_CLASS(drill::RateLimitFilter, drill::VelocityFilter)
PLUGINLIB_EXPORT_CLASS(drill::DeadbandFilter, drill::VelocityFilter)
```

The table of names.

```xml
<?xml version="1.0"?>
<!-- ~/ros2_ws/src/velocity_filter_demo/plugins.xml -->
<library path="velocity_filters">
  <class name="drill/ClampFilter" type="drill::ClampFilter"
         base_class_type="drill::VelocityFilter">
    <description>Clamps the absolute value of the speeds.</description>
  </class>
  <class name="drill/RateLimitFilter" type="drill::RateLimitFilter"
         base_class_type="drill::VelocityFilter">
    <description>Limits the change from the previous output.</description>
  </class>
  <class name="drill/DeadbandFilter" type="drill::DeadbandFilter"
         base_class_type="drill::VelocityFilter">
    <description>Sets small speeds to zero.</description>
  </class>
</library>
```

The host. It loads the plugins in the order of `filters` in the YAML, passes three commands through them in order, and prints the results.

```cpp
// ~/ros2_ws/src/velocity_filter_demo/src/filter_host.cpp
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "drill/velocity_filter.hpp"
#include "pluginlib/class_loader.hpp"
#include "rclcpp/rclcpp.hpp"

namespace drill
{

// Loads plugins in the order of "filters" in the YAML and passes a command through them in order.
class FilterPipeline
{
public:
  explicit FilterPipeline(const rclcpp::Node::SharedPtr & node)
  : loader_("velocity_filter_demo", "drill::VelocityFilter")
  {
    // The default is an empty list (no filters = the command passes unchanged).
    const auto filter_names =
      node->declare_parameter<std::vector<std::string>>("filters", std::vector<std::string>{});
    for (const auto & filter_name : filter_names) {
      filters_.push_back(loadFilter(node, filter_name));
    }
  }

  geometry_msgs::msg::Twist apply(const geometry_msgs::msg::Twist & command, double dt_seconds)
  {
    geometry_msgs::msg::Twist result = command;
    for (const auto & filter : filters_) {
      result = filter->filter(result, dt_seconds);
    }
    return result;
  }

  std::vector<std::string> declaredClasses() {return loader_.getDeclaredClasses();}

private:
  pluginlib::UniquePtr<VelocityFilter> loadFilter(
    const rclcpp::Node::SharedPtr & node, const std::string & filter_name)
  {
    const auto plugin_name = node->declare_parameter<std::string>(filter_name + ".plugin", "");
    if (plugin_name.empty()) {
      throw std::runtime_error(
              "filter '" + filter_name + "': parameter '" + filter_name + ".plugin' is not set");
    }
    try {
      auto filter = loader_.createUniqueInstance(plugin_name);
      filter->initialize(node, filter_name);
      return filter;
    } catch (const pluginlib::PluginlibException & exception) {
      throw std::runtime_error(
              "filter '" + filter_name + "': cannot load '" + plugin_name + "': " + exception.what());
    }
  }

  // Members are destroyed in reverse order of declaration. Declare loader_ before filters_
  // so that the ClassLoader outlives the instances.
  pluginlib::ClassLoader<VelocityFilter> loader_;
  std::vector<pluginlib::UniquePtr<VelocityFilter>> filters_;
};

}  // namespace drill

int main(int argc, char ** argv)
{
  constexpr double kStepSeconds = 0.1;

  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("filter_host");
  int exit_code = 0;
  try {
    drill::FilterPipeline pipeline(node);

    std::cout << "declared:";
    for (const auto & class_name : pipeline.declaredClasses()) {
      std::cout << " " << class_name;
    }
    std::cout << std::endl;

    const std::vector<std::pair<double, double>> commands = {{0.8, 0.05}, {0.8, 0.5}, {0.02, 0.5}};
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t index = 0; index < commands.size(); ++index) {
      geometry_msgs::msg::Twist command;
      command.linear.x = commands[index].first;
      command.angular.z = commands[index].second;
      const auto result = pipeline.apply(command, kStepSeconds);
      std::cout << "step " << index + 1 << ": in (" << command.linear.x << ", " << command.angular.z
                << ") -> out (" << result.linear.x << ", " << result.angular.z << ")" << std::endl;
    }
  } catch (const std::exception & exception) {
    RCLCPP_ERROR(node->get_logger(), "%s", exception.what());
    exit_code = 1;
  }
  rclcpp::shutdown();
  return exit_code;
}
```

Finally, the YAML we saw in the design section.

```yaml
# ~/ros2_ws/src/velocity_filter_demo/config/filters.yaml
filter_host:
  ros__parameters:
    filters: ["deadband", "clamp", "rate_limit"]   # applied in this order
    deadband:
      plugin: "drill/DeadbandFilter"
      linear_threshold: 0.05
    clamp:
      plugin: "drill/ClampFilter"
      max_linear: 0.5
    rate_limit:
      plugin: "drill/RateLimitFilter"
      max_linear_acceleration: 1.0
```

### Run it

Build it, and run it with the YAML.

```bash
cd ~/ros2_ws
colcon build --packages-select velocity_filter_demo
source install/setup.bash
ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml
```

**Predict: Three commands `(0.8, 0.05)`, `(0.8, 0.5)`, and `(0.02, 0.5)` (`linear.x`, `angular.z`) pass through `deadband`, `clamp`, and `rate_limit` in this order. What does each one become? (One step is 0.1 seconds. `rate_limit` has `max_linear_acceleration` of 1.0, and `max_angular_acceleration` is not written, so it is the default 1.0.)**

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml
declared: drill/ClampFilter drill/DeadbandFilter drill/RateLimitFilter
step 1: in (0.80, 0.05) -> out (0.10, 0.00)
step 2: in (0.80, 0.50) -> out (0.20, 0.10)
step 3: in (0.02, 0.50) -> out (0.10, 0.20)
```

The first line is the list of registered names that `getDeclaredClasses()` returned.

- Step 1: The `angular.z` of `0.05` is smaller than the `deadband` threshold `0.1`, so it becomes `0`. The `linear.x` of `0.8` is clamped to `0.5` by `clamp`, and then `rate_limit` limits the change from 0 to `1.0 x 0.1 = 0.1`, so it becomes `0.10`
- Step 2: `linear.x` rises by `0.1` from the previous `0.10` to `0.20`, and `angular.z` rises by `0.1` from `0` to `0.10`
- Step 3: The `linear.x` of `0.02` becomes `0` in `deadband`, and `rate_limit` lowers it by `0.1` from `0.20` to `0.10`. `angular.z` rises from `0.10` to `0.20`

`filter_host` never includes an implementation header, yet it ran the three filters. The implementations are in the `velocity_filters` library, and `filter_host` loads them at run time by name only.

</details>

### Change the order

If you change the order of `filters` in the YAML, the same commands give different results. We override only `filters` from the command line with `-p`.

```bash
ros2 run velocity_filter_demo filter_host --ros-args \
  --params-file src/velocity_filter_demo/config/filters.yaml \
  -p 'filters:=[rate_limit, clamp, deadband]'
```

**Predict: With `rate_limit` first, what does `angular.z` become in step 2?**

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[rate_limit, clamp, deadband]'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p 'filters:=[rate_limit, clamp, deadband]'" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[rate_limit, clamp, deadband]
declared: drill/ClampFilter drill/DeadbandFilter drill/RateLimitFilter
step 1: in (0.80, 0.05) -> out (0.10, 0.00)
step 2: in (0.80, 0.50) -> out (0.20, 0.15)
step 3: in (0.02, 0.50) -> out (0.10, 0.25)
```

The first `rate_limit` remembers the `angular.z` from before `deadband` removes it (`0.05` in step 1) as its previous output. So in step 2 it rises by `0.1` from `0.05` to `0.15`, which is above the `deadband` threshold `0.1` and survives. This is what "the order matters" means. That is why we state the order explicitly in the `filters` array.

</details>

### See the failures

If you know how to read the messages, plugin failures are easy to fix. Let us cause them.

#### Loading an unknown name

Override the `plugin` of `clamp` with a name that is not registered.

```bash
ros2 run velocity_filter_demo filter_host --ros-args \
  --params-file src/velocity_filter_demo/config/filters.yaml \
  -p clamp.plugin:=drill/NoSuchFilter
```

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p clamp.plugin:=drill/NoSuchFilter'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p clamp.plugin:=drill/NoSuchFilter" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p clamp.plugin:=drill/NoSuchFilter
[ERROR] [1791032328.608482387] [filter_host]: filter 'clamp': cannot load 'drill/NoSuchFilter': According to the loaded plugin descriptions the class drill/NoSuchFilter with base class type drill::VelocityFilter does not exist. Declared types are  drill/ClampFilter drill/DeadbandFilter drill/RateLimitFilter
[ros2run]: Process exited with failure 1
```

The `what()` of `PluginlibException` says "no class with that name exists for the base class `drill::VelocityFilter`", and it gives **the list of registered names** (after `Declared types are`). The host adds which filter (`clamp`) it was, and stops with exit code 1. If it is a typo, you can see it at once by comparing with the list.

If you forget `plugin`, the host checks it by itself and prints a different message.

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[deadband, mystery]'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p 'filters:=[deadband, mystery]'" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[deadband, mystery]
[ERROR] [1791032315.152743755] [filter_host]: filter 'mystery': parameter 'mystery.plugin' is not set
[ros2run]: Process exited with failure 1
```

This is the case where we added `mystery` to `filters` but did not write `mystery.plugin`.

#### Forgetting to export plugins.xml

Delete the `pluginlib_export_plugin_description_file` line from CMakeLists.txt and build again. The build succeeds. But when you run it, no plugin is found.

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="sed -i '/pluginlib_export_plugin_description_file/d' src/velocity_filter_demo/CMakeLists.txt; colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml
[ERROR] [1791032303.100193333] [filter_host]: filter 'deadband': cannot load 'drill/DeadbandFilter': According to the loaded plugin descriptions the class drill/DeadbandFilter with base class type drill::VelocityFilter does not exist. Declared types are
[ros2run]: Process exited with failure 1
```

The message has the same form as the previous "unknown name", but **nothing** follows `Declared types are`. It means the name is not wrong: `plugins.xml` is not registered. When you cannot see any plugin, suspect this first. When you call `pluginlib_export_plugin_description_file`, an index named `<package_name>__pluginlib__plugin` is created under `install`. You can check the build result with the following command.

```bash
ls install/velocity_filter_demo/share/ament_index/resource_index | grep pluginlib
```

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; echo '$ ls install/velocity_filter_demo/share/ament_index/resource_index | grep pluginlib'; ls install/velocity_filter_demo/share/ament_index/resource_index | grep pluginlib" -->
```
$ ls install/velocity_filter_demo/share/ament_index/resource_index | grep pluginlib
velocity_filter_demo__pluginlib__plugin
```

If this line does not appear, `pluginlib_export_plugin_description_file` was not called.

#### Destroying the ClassLoader first

Swap the declaration order of the two members in `filter_host.cpp` (`filters_` first, `loader_` after).

```cpp
  std::vector<pluginlib::UniquePtr<VelocityFilter>> filters_;       // declared first = destroyed last
  pluginlib::ClassLoader<VelocityFilter> loader_;                   // declared later = destroyed first (bad)
```

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="sed -i -e '/^  pluginlib::ClassLoader<VelocityFilter> loader_;/{h;d}' -e '/^  std::vector<pluginlib::UniquePtr<VelocityFilter>> filters_;/G' src/velocity_filter_demo/src/filter_host.cpp; colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml
declared: drill/ClampFilter drill/DeadbandFilter drill/RateLimitFilter
step 1: in (0.80, 0.05) -> out (0.10, 0.00)
step 2: in (0.80, 0.50) -> out (0.20, 0.10)
step 3: in (0.02, 0.50) -> out (0.10, 0.20)
Warning: class_loader.ClassLoader: SEVERE WARNING!!! Attempting to unload library while objects created by this loader exist in the heap! You should delete your objects before attempting to unload the library or destroying the ClassLoader. The library will NOT be unloaded.
         at line 127 in ./src/class_loader.cpp
```

In this measurement, the results were printed correctly, and `class_loader` printed a warning at exit. It says "objects are still alive, so the library will not be unloaded". `class_loader` kept the library loaded so that the program does not crash, but the warning is a sign that the contract (the `ClassLoader` outlives the instances) was broken. Keep the order that gives no warning.

#### Getting the YAML type wrong

If you drop the decimal point, as in `max_linear: 1`, it becomes an integer. It does not fit a parameter declared as `double`.

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p clamp.max_linear:=1'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p clamp.max_linear:=1" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p clamp.max_linear:=1
[ERROR] [1791032259.421103799] [filter_host]: parameter 'clamp.max_linear' has invalid type: Wrong parameter type, parameter {clamp.max_linear} is of type {double}, setting it to {integer} is not allowed.
[ros2run]: Process exited with failure 1
```

If you write `1.0`, it passes.

#### Using the same name twice

If you write the same name twice in `filters`, it fails at the second `declare_parameter`. If you want to use the same class twice, give it another name (and another namespace).

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[clamp, clamp]'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p 'filters:=[clamp, clamp]'" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[clamp, clamp]
[ERROR] [1791032245.727764001] [filter_host]: parameter 'clamp.plugin' has already been declared
[ros2run]: Process exited with failure 1
```

## Common pitfalls

- **Writing `plugins.xml` is not enough.** Export it with `pluginlib_export_plugin_description_file` in CMakeLists.txt, and write the `pluginlib` dependency in `package.xml`. If you forget, you get the "the list of registered names is empty" message
- **Match the names in three places.** `type` and `base_class_type` in `plugins.xml` must match the two arguments of `PLUGINLIB_EXPORT_CLASS`, including the namespace. The second argument of the host's `ClassLoader` must also be the same as `base_class_type`
- **`library path` is the library name without `lib` and `.so`.** It is the same as the name in `add_library`
- **The first argument of `ClassLoader` is the package that has the base class.** It is not the package where you wrote the plugins. The first argument of `pluginlib_export_plugin_description_file` follows the same idea
- **You cannot receive settings in the constructor.** Objects are created with no arguments, so receive them in `initialize()`
- **Declare the `ClassLoader` before the members that hold instances.** With the opposite order, a warning appears at exit, as measured. When you use it as local variables, create the `ClassLoader` first
- **`ament_target_dependencies` was removed in Lyrical.** Write it with `target_link_libraries`
- **Match the type of YAML numbers.** Write `1.0` for a `double` parameter
- **Give each plugin its own namespace.** If the names collide, it fails with an already-declared exception

## Conclusion

pluginlib is a mechanism for "deciding a base class and a table of names in advance, and choosing the implementation from configuration". The pipeline in this chapter is built in the same form as the nav2 planner and controller (a list of names, and a namespace for each name). The next time you read the YAML of a standard package, follow what the `plugin:` line selects, and what the base class of that class is.

Next, in [16_implementing_an_action_server](16_implementing_an_action_server.md), we implement a long-running task with feedback on the server side. If anything is unclear, ask someone experienced or check the official documentation.

### Matching exercise

After reading this chapter, work on the matching drills.

- `16_pluginlib_create` — Create a plugin and export it
- `17_pluginlib_load` — Load it with ClassLoader
- `18_pluginlib_yaml` — Build a chain of plugins from YAML parameters

```bash
./drill run 16
./drill run 17
./drill run 18
```

From an exercise, you can come back to this chapter with `./drill read`.

## References

- [Creating and using plugins (C++) — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
- [pluginlib — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/p/pluginlib/)
- [Writing a new hardware component — ros2_control Documentation: Jazzy](https://control.ros.org/jazzy/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html)
- [Planner Server — Nav2 Documentation](https://docs.nav2.org/rolling/configuration_and_development/configuration_guide/core_servers/configuring_planner_server/)
- [04_nodes](04_nodes.md)
- Previous: [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md)
- Next: [16_implementing_an_action_server](16_implementing_an_action_server.md)
