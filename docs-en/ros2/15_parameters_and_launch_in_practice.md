# ROS 2 Lecture 15: Parameters and Launch in Practice

## Introduction

After this article, you will be able to give a node a parameter such as `max_speed`, and start it with a combination of a YAML file and a launch file. You will also be able to build a node whose internal behavior really changes when you change a value at run time with `ros2 param set`.

In [07_parameters](07_parameters.md), we saw the problem that "I changed the value with `ros2 param set`, but the behavior did not change". That happened because the node did not implement a callback to receive parameter changes. Here we implement it. The launch file in [09_launch_and_ros2_bag](09_launch_and_ros2_bag.md) was a minimal one that only embedded a remap. Here we go further, with `DeclareLaunchArgument` and YAML injection.

The prerequisite is that you have finished [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md). A node with parameters has almost the same shape as the Python pub/sub node, so you must be able to write a node with rclpy.

## Goals

- Write a node that has parameters, using `declare_parameter` / `get_parameter`
- Detect parameter changes at run time with `add_on_set_parameters_callback` and apply them to internal variables
- Install a parameter YAML file in the package, and load it from a launch file with `parameters=[...]`
- Handle launch arguments with `DeclareLaunchArgument` / `LaunchConfiguration`

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco already set up
- The workspace created in [10_workspaces_and_colcon](10_workspaces_and_colcon.md)
- A package for this lecture (Python, ament_python). You can create it on the day, but try `ros2 pkg create --build-type ament_python speed_param_demo` once beforehand to learn where people get stuck
- Indentation mistakes in YAML files often cause `--params-file` to be ignored, so keep a YAML file that you have tested on the teacher's machine too

### Suggested time plan

- Explain the parameter change callback and implement the code: 15 min
- Create the YAML file and add the install setting to setup.py: 10 min
- Create the launch file (Node/DeclareLaunchArgument): 15 min
- Exercises (change YAML and restart, check changes with param set): 15 min
- Oral questions: 10 min

### Oral questions

**Q1. What happens if you run `ros2 param set` on a node that implements only `declare_parameter` and does not implement `add_on_set_parameters_callback`?**

Model answer: The value on the parameter server is changed, and `ros2 param get` returns the new value. But the internal variable of the node (the variable that holds the value read with `get_parameter` in the constructor) is not updated automatically, so the new value does not affect the real processing. "The value looks changed" and "the behavior reflects the change" are different things. To get the latter, you need code in the callback that updates the internal variable explicitly.

**Q2. What is the difference between writing `parameters=['config/speed_param.yaml']` directly in the launch file's `Node`, and installing the YAML with `setup.py` and then passing the path `os.path.join(get_package_share_directory(...), 'config', 'speed_param.yaml')`?**

Model answer: Writing a relative path directly makes the behavior depend on the directory from which you run `ros2 launch`, and it fails when you start it from another working directory. Using the install path obtained with `get_package_share_directory` finds the same YAML wherever you run it, once the package has been installed by `colcon build`. If you think about distribution and shared use, the latter is the correct way, and you must include the YAML in the install targets with `data_files` in `setup.py`.

**Q3. Imagine you need to adjust `max_speed` at a test-run site. Compare two methods: rewriting the default value of `declare_parameter('max_speed', 1.0)` in the source code and rebuilding, or rewriting only the value in the YAML file that launch loads and restarting. Which one suits the site?**

Model answer: Rewriting only the YAML value suits the site. The former adds the steps source change, `colcon build`, and reflecting the install, and the build often takes tens of seconds to several minutes. The latter only needs you to fix the number in the YAML with a text editor and run `ros2 launch` again. At a test-run site, these few minutes add up and reduce the total number of tuning attempts, so it is better to design the parameters to come from YAML and not to embed them in the code.

## Main text

### What you learn: Writing a node with parameters

What you learn: declare a parameter with `declare_parameter` and read it with `get_parameter`.

Preparation: create the Python package `speed_param_demo` in the workspace.

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_python speed_param_demo
```

Details:

Create `speed_param_demo/speed_param_demo/speed_node.py` with the following content. It is a simple node that has a parameter `max_speed` and only logs the current value at a fixed period with a timer.

```python
import rclpy
from rclpy.node import Node


class SpeedNode(Node):
    def __init__(self):
        super().__init__('speed_node')

        # Declare it with the default value 1.0. A parameter that is not declared
        # raises an exception in get_parameter, so always declare the parameters you use.
        self.declare_parameter('max_speed', 1.0)

        # Keep the value at constructor time in an internal variable.
        self.max_speed = self.get_parameter('max_speed').get_parameter_value().double_value

        self.create_timer(1.0, self.on_timer)

    def on_timer(self):
        self.get_logger().info(f'max_speed = {self.max_speed}')


def main():
    rclpy.init()
    node = SpeedNode()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

Hint: `get_parameter('max_speed').get_parameter_value().double_value` looks a little long, but it is a way of writing that names the parameter type (int/double/string/bool, and so on) explicitly. You may also see code examples that use only `.value` without thinking about the type, but that makes unintended type conversions hard to notice, so get used to the style that names the type.

Register it in `entry_points` of `setup.py`, and after the build check that it starts on its own.

```bash
colcon build --packages-select speed_param_demo
source install/setup.bash
ros2 run speed_param_demo speed_node
```

If the log `max_speed = 1.0` appears every second, it works. In another terminal, run `ros2 param set /speed_node max_speed 5.0`. `ros2 param get /speed_node max_speed` returns 5.0, but the log still shows 1.0. This is the "set has no effect" phenomenon mentioned in [07_parameters](07_parameters.md).

### What you learn: Implementing a parameter change callback

What you learn: receive changes at run time with `add_on_set_parameters_callback` and update the internal variable.

Preparation: the node from exercise 1 must be able to start.

Details:

Add the callback registration to the constructor of `speed_node.py`.

```python
# ~/ros2_ws/src/speed_param_demo/speed_param_demo/speed_node.py
import rclpy
from rclpy.node import Node
from rcl_interfaces.msg import SetParametersResult


class SpeedNode(Node):
    def __init__(self):
        super().__init__('speed_node')

        self.declare_parameter('max_speed', 1.0)
        self.max_speed = self.get_parameter('max_speed').get_parameter_value().double_value

        # on_parameter_change is now called every time a parameter changes.
        self.add_on_set_parameters_callback(self.on_parameter_change)

        self.create_timer(1.0, self.on_timer)

    def on_parameter_change(self, params):
        for param in params:
            if param.name == 'max_speed':
                # You can also write checks here, such as not allowing a negative speed.
                if param.value < 0.0:
                    return SetParametersResult(successful=False, reason='max_speed must be >= 0')
                self.max_speed = param.value
        return SetParametersResult(successful=True)

    def on_timer(self):
        self.get_logger().info(f'max_speed = {self.max_speed}')


def main():
    rclpy.init()
    node = SpeedNode()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

The function registered with `add_on_set_parameters_callback` runs every time `ros2 param set` is called, and it receives the list of parameters being changed (`params`). If you return `SetParametersResult(successful=True)`, the change is committed. If you return `successful=False`, the change is rejected and the old value stays. Here we also add a check that rejects `max_speed` when it is about to become negative. The core of this exercise is that you must always write, as a pair, the code that updates the variable that really controls the robot's behavior (in this example, `self.max_speed`), and not only the value on the parameter server.

Rebuild and run it.

```bash
colcon build --packages-select speed_param_demo
source install/setup.bash
ros2 run speed_param_demo speed_node
```

Change the value from another terminal.

```bash
ros2 param set /speed_node max_speed 5.0
```

This time the log switches to `max_speed = 5.0`. Now try a negative value.

```bash
ros2 param set /speed_node max_speed -1.0
```

The command fails, and even if you check with `ros2 param get /speed_node max_speed`, the value is still 5.0. This shows that the validation works.

> Column: Why write the validation on the callback side
>
> A value such as `max_speed` has a lower and an upper limit set by the motor driver specification and the physical limits of the robot. If an out-of-range value goes straight into the internal variable, one moment of typing error at a test-run site can make the robot run out of control. If you can reject it in the callback, the robot side will reject the value even when the person typing `ros2 param set` mistypes a number. For a robot in a robot contest, treat this check as "required", not as "nice to have".

### What you learn: Installing a parameter YAML in the package

What you learn: add the YAML to `data_files` in `setup.py` and load it with `get_package_share_directory`.

Preparation: create a `config` directory in the package from exercise 2.

```bash
cd ~/ros2_ws/src/speed_param_demo
mkdir config
```

Details:

Create `config/speed_param.yaml`. It has a structure keyed by the node name, and the key structure is fixed: the node name comes first, and `ros__parameters` comes under it.

```yaml
# ~/ros2_ws/src/speed_param_demo/config/speed_param.yaml
speed_node:
  ros__parameters:
    max_speed: 2.0
```

Edit `setup.py` and add to `data_files` so that the contents of the `config` directory are placed in the install location.

```python
# ~/ros2_ws/src/speed_param_demo/setup.py
import os
from glob import glob
from setuptools import setup

package_name = 'speed_param_demo'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        # Copy the launch files and YAML files under share/
        (os.path.join('share', package_name, 'launch'), glob('launch/*.py')),
        (os.path.join('share', package_name, 'config'), glob('config/*.yaml')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    entry_points={
        'console_scripts': [
            'speed_node = speed_param_demo.speed_node:main',
        ],
    },
)
```

If you write `glob('config/*.yaml')`, you do not need to rewrite the list every time you add or remove a YAML file. The `launch` directory is added in the same way (you will use it right away in the next exercise).

Build, and check that the YAML was really copied to the install location.

```bash
colcon build --packages-select speed_param_demo
ls install/speed_param_demo/share/speed_param_demo/config/
```

If `speed_param.yaml` is shown, the install setting works. Hint: if it is not shown, the way `data_files` is written in `setup.py` is the first thing to suspect. `colcon build` may simply forget to copy the files without any error, so always get into the habit of checking with `ls` by eye.

Also check that you can start it by giving the file directly with `--params-file`.

```bash
ros2 run speed_param_demo speed_node --ros-args --params-file install/speed_param_demo/share/speed_param_demo/config/speed_param.yaml
```

If the log `max_speed = 2.0` appears, the YAML value was loaded correctly.

### What you learn: Injecting the YAML from a launch file

What you learn: pass the YAML file with the `parameters=` argument of the `Node` action.

Preparation: the YAML from exercise 3 must be installed.

Details:

Create `launch/speed_param_launch.py`.

```python
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory('speed_param_demo'),
        'config',
        'speed_param.yaml',
    )

    return LaunchDescription([
        Node(
            package='speed_param_demo',
            executable='speed_node',
            name='speed_node',
            parameters=[config],
        ),
    ])
```

`get_package_share_directory('speed_param_demo')` returns the absolute path of the share directory of this package, as installed by colcon. By joining `config/speed_param.yaml` to it, you build a path that points to the same YAML file wherever you run `ros2 launch` from. For `parameters=` of `Node`, besides the path of a YAML file, you can also pass a dictionary (`{'max_speed': 2.0}`) directly. But considering how easy it is to rewrite at a test-run site, it is easier to handle if you use YAML for everything.

`setup.py` already has the install setting for the `launch` directory, so rebuild and run.

```bash
colcon build --packages-select speed_param_demo
source install/setup.bash
ros2 launch speed_param_demo speed_param_launch.py
```

If the log `max_speed = 2.0` appears, the injection through YAML worked. Now press Ctrl+C once, change the `max_speed` value in the YAML to `8.0`, and run the same `ros2 launch` command again. Check that the log changes to `max_speed = 8.0`. You did not have to change a single character of the source code.

### What you learn: Switching start-up values with launch arguments

What you learn: use `DeclareLaunchArgument` and `LaunchConfiguration` to pass remapping and a namespace from command-line arguments.

Preparation: the launch file from exercise 4 must work.

Details:

Imagine you want to start several robots with the same launch file, and make `namespace` specifiable from the command line.

```python
# ~/ros2_ws/src/speed_param_demo/launch/speed_param_launch.py
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory('speed_param_demo'),
        'config',
        'speed_param.yaml',
    )

    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Namespace of the node. Use it to tell several robots apart',
    )

    return LaunchDescription([
        namespace_arg,
        Node(
            package='speed_param_demo',
            executable='speed_node',
            name='speed_node',
            namespace=LaunchConfiguration('namespace'),
            parameters=[config],
            remappings=[
                ('/cmd_vel', 'cmd_vel'),
            ],
        ),
    ])
```

`DeclareLaunchArgument('namespace', default_value='')` defines an argument that you can pass as `namespace:=value` when you run `ros2 launch`. `LaunchConfiguration('namespace')` is a placeholder that is resolved at run time, and when you pass it to `namespace=` of `Node`, it is fixed at start-up. With the default value set to an empty string, the node starts without a namespace if you omit the argument.

**Here is a trap that you should step on once.** The top-level key of the `speed_param.yaml` we made above is `speed_node:` (no namespace), but if you start with `namespace:=robot1`, the fully qualified name of the node becomes `/robot1/speed_node`. A parameter YAML matches the node name including the namespace, so the key `speed_node:` does **not match** `/robot1/speed_node`. It is not an error, and the node starts with the default value of `declare_parameter`.

<!-- measure: env=ros files=src/speed_param_demo/speed_param_demo/speed_node.py,src/speed_param_demo/config/speed_param.yaml,src/speed_param_demo/setup.py,src/speed_param_demo/launch/speed_param_launch.py cmd="export PYTHONUNBUFFERED=1; ros2 pkg create --build-type ament_python --destination-directory /tmp/generated speed_param_demo >/dev/null 2>&1; cp -rn /tmp/generated/speed_param_demo/. src/speed_param_demo/ 2>/dev/null; colcon build --packages-select speed_param_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 launch speed_param_demo speed_param_launch.py'; timeout -s INT 4 ros2 launch speed_param_demo speed_param_launch.py 2>&1 | grep -m1 max_speed; echo '$ ros2 launch speed_param_demo speed_param_launch.py namespace:=robot1'; timeout -s INT 4 ros2 launch speed_param_demo speed_param_launch.py namespace:=robot1 2>&1 | grep -m1 max_speed" -->
```
$ ros2 launch speed_param_demo speed_param_launch.py
[speed_node-1] [INFO] [1790939696.049941586] [speed_node]: max_speed = 2.0
$ ros2 launch speed_param_demo speed_param_launch.py namespace:=robot1
[speed_node-1] [INFO] [1790939699.503564004] [robot1.speed_node]: max_speed = 1.0
```

The top one is with the YAML taking effect (`2.0`), the bottom one is still the default value (`1.0`).

If you want to reuse the same YAML for several robots, make the top-level key the wildcard `/**`. Then it applies even when a namespace is added.

```yaml
/**:
  ros__parameters:
    max_speed: 2.0
```

When "I passed the YAML but the value did not change", first check the real value with `ros2 param get /<namespace>/<node name> <parameter name>`, and then check whether the key in the YAML matches the fully qualified name.

If you start it without the argument, it is the same as before.

```bash
ros2 launch speed_param_demo speed_param_launch.py
```

If you start it with the argument, the node name gets a namespace, like `/robot1/speed_node`.

```bash
ros2 launch speed_param_demo speed_param_launch.py namespace:=robot1
```

Run `ros2 node list` in another terminal, and check that you can see `/robot1/speed_node`.

**Practice problem**: Change this launch file so that the path of the YAML file itself can be switched with a launch argument. Keep the default as the current `config/speed_param.yaml`, and make it possible to give another YAML path with `params_file:=`.

<details markdown="1"><summary>Answer</summary>

```python
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    default_config = os.path.join(
        get_package_share_directory('speed_param_demo'),
        'config',
        'speed_param.yaml',
    )

    params_file_arg = DeclareLaunchArgument(
        'params_file',
        default_value=default_config,
        description='Path of the parameter YAML file to load',
    )

    return LaunchDescription([
        params_file_arg,
        Node(
            package='speed_param_demo',
            executable='speed_node',
            name='speed_node',
            parameters=[LaunchConfiguration('params_file')],
        ),
    ])
```

By passing the default YAML path to `default_value` and writing `parameters=[LaunchConfiguration('params_file')]`, it loads the usual file if you omit the argument, and it loads another YAML if you give one (for example `params_file:=/home/user/tuned_speed_param.yaml`). This form is useful when you keep several tuning patterns as YAML files at a test-run site and want to switch between them depending on the situation.

</details>

## Going further

The node this time had only one value, `max_speed`. A real robot contest robot handles dozens of parameters, such as control gains (Kp/Ki/Kd), robot dimensions, and frame IDs. When parameters increase, you will want to put the settings of all nodes in one YAML, and let several launch files read only part of the same YAML. For this, there is the way of using the wildcard (`/**`) for the node name in the YAML, and the way of handling a group of nodes together with `launch.actions.GroupAction`.


Python parameter types can also handle implicit lists (array parameters such as `declare_parameter('gains', [1.0, 0.1, 0.01])`), but this is a part where a wrong type specification easily causes an error at start-up. When you start to use array parameters seriously, read the types chapter of the official documentation again.

## Conclusion

When you can combine parameters and launch, adjustment at a test-run site is freed from rebuilding the source code. The ideal is a setup where the behavior changes by "fixing only the YAML of launch and restarting". On the other hand, if you bring parameters hard-coded in the code to the site, every small adjustment on the spot makes you wait for a build. Remember these two points as a set, as in the `speed_node` we made this time: always declare the values you want to adjust with `declare_parameter`, and always reflect the values you want to take effect at run time into internal variables with `add_on_set_parameters_callback`.

Next, in [16_implementing_an_action_server](16_implementing_an_action_server.md), we implement a long-running task with feedback on the server side. If anything is unclear, ask someone experienced or check the official documentation.

### Matching exercise

After reading this chapter, practice with the matching drills.

- `06_parameters` — Use parameters inside a class
- `07_param_events` — Watch parameter changes
- `08_params_yaml` — Manage parameters with YAML
- `09_launch` — Write launch in Python / XML / YAML

```bash
./drill run 06
./drill run 07
./drill run 08
./drill run 09
```

From an exercise, you can come back to this chapter with `./drill read`.

## References

- [Using parameters in a class (Python) — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Using-Parameters-In-A-Class-Python.html)
- [Launch — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Launch/Launch-Main.html)
- [07_parameters](07_parameters.md)
- [09_launch_and_ros2_bag](09_launch_and_ros2_bag.md)
- Previous: [14_implementing_services](14_implementing_services.md)
- Next: [16_implementing_an_action_server](16_implementing_an_action_server.md)
