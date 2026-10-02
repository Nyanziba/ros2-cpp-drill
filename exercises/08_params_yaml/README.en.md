# Exercise 08: Manage parameters with YAML [Advanced]

You write the "parameter YAML file" described in the official documents
[Understanding parameters](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Parameters/Understanding-ROS2-Parameters.html)
and
[About parameters](https://docs.ros.org/en/jazzy/Concepts/Basic/About-Parameters.html)
yourself.

In exercises 06 and 07 you handled parameters from the code side, but this time you write no code at all.
You write only two files: `config/params.yaml` and `launch/param_demo.launch.py`.
Grading is also not by a C++ gtest but by **pytest**, which actually starts the node and checks it
(`src/param_echo.cpp` is already complete and you do not edit it).

## What to do

Fill in the TODOs in `config/params.yaml` and `launch/param_demo.launch.py`.

Set the following four parameters for the node `param_echo` in YAML.

| Parameter name | Type | Value |
| --- | --- | --- |
| `my_parameter` | string | `bonjour` |
| `an_int_param` | integer | `7` |
| `a_double_param` | floating point | `1.5` |
| `a_string_list` | list of strings | `["alpha", "beta"]` |

`launch/param_demo.launch.py` is a launch file that loads this YAML and starts `param_echo`.

## The 3-level structure of a parameter YAML

Every parameter YAML has the following 3-level structure. This is where many people make mistakes.

```yaml
<node name>:
  ros__parameters:
    <parameter name>: <value>
```

- The first level is the node name. Write the name that appears in `ros2 node list` as it is.
- The second level is always the fixed key `ros__parameters`.
  Note that it has **two underscores**
  (it is not `ros_parameters`. Using one underscore is a very common mistake).
- At the third level, you list parameter names and values.

### A node with a namespace

If the node has a namespace, write the first level like `/ns/node_name`.

```yaml
/my_ns/param_echo:
  ros__parameters:
    my_parameter: bonjour
```

### The wildcard `/**` for all nodes

If you put `/**` at the first level instead of a node name, the same parameters are applied to **all nodes**
that load the file (if a specific node name is given, that one takes priority).
In this exercise, use the concrete node name `param_echo`, not the wildcard.

## The YAML type becomes the parameter type

The type of the value you write in YAML becomes the type of the parameter as it is.

| How you write it in YAML | Parameter type |
| --- | --- |
| `7` | integer |
| `7.0` | floating point |
| `"7"` | string |
| `[1, 2]` | array of integers |

**If the type does not match, `declare_parameter()` on the node side throws an exception and the node crashes.**
If you write `my_parameter` as `bonjour` (without quotes), it is still read as a string, but
if you quote `an_int_param` like `"7"`, it becomes a string,
and since `param_echo` declares it as `int64_t`, it throws an exception at startup.
`a_double_param` becomes a floating point only if you write the decimal point, as in `1.5`
(if you write `1`, it becomes an integer).

## Three ways to use it

You can load the same YAML file in the following three ways.

1. Directly from the command line:
   ```bash
   ros2 run drill_08_params_yaml param_echo --ros-args --params-file config/params.yaml
   ```
2. From `parameters=[...]` in a launch file:
   ```python
   Node(
       package='drill_08_params_yaml',
       executable='param_echo',
       parameters=[params_file],
   )
   ```
3. Load it later into a running node with `ros2 param load`:
   ```bash
   ros2 param load /param_echo config/params.yaml
   ```

## Write the current values out to YAML

In the other direction, you can also write the values a running node currently has out as YAML.

```bash
ros2 param dump /param_echo
```

YAML with the same 3-level structure is printed to standard output. You can also use it as a
model when you write YAML by hand.

## Run it

After the tests pass, you can run it by hand and check.

```bash
source install/setup.bash
ros2 run drill_08_params_yaml param_echo --ros-args --params-file config/params.yaml
```

`param_echo` prints four lines of log and exits immediately.

```
[INFO] [...] [param_echo]: my_parameter=bonjour
[INFO] [...] [param_echo]: an_int_param=7
[INFO] [...] [param_echo]: a_double_param=1.5
[INFO] [...] [param_echo]: a_string_list=[alpha,beta]
```

You can do the same through launch.

```bash
ros2 launch drill_08_params_yaml param_demo.launch.py
```

## Common pitfalls

- `ros__parameters` has **two** underscores. If you use one, it is treated as
  "that key does not exist", and no parameter is loaded at all.
- Do not put quotes around a number, as in `an_int_param: 7`.
  If you write `"7"`, it becomes a string, does not match the type on the node side, and throws an exception.
- Write `a_double_param` with the decimal point, as in `1.5`. If you write `1`, it becomes an integer.
- `get_package_share_directory()` looks at the `share/` directory after installation.
  Even if you edit `config/params.yaml`, if you do not run `colcon build`, the launch side gets
  the old contents (or a file that does not exist).

## Tests

```bash
./drill run 08
```

| Test | What it checks |
| --- | --- |
| `testパラメータYAMLが構文として読み込める` (test the parameter YAML can be loaded as valid syntax) | Whether `config/params.yaml` can be read as YAML without syntax errors |
| `test_3段構造になっている_ノード名からros__parametersまで` (test it has the 3-level structure, from the node name to ros__parameters) | Whether the hierarchy is `param_echo` → `ros__parameters` |
| `test_4つのパラメータが正しい型と値になっている` (test the four parameters have the correct types and values) | Whether the types and values of the four parameters match the specification |
| `testEndToEndでparam_echoが正しい値をログに出す` (test End-to-End that param_echo prints the correct values in the log) | Whether it actually starts `param_echo` and the correct values appear in the log |
| `test_launchファイルがparam_echoをparams_yaml付きで起動する` (test the launch file starts param_echo with params_yaml) | Whether launch starts `param_echo` with `config/params.yaml` |

## References

- Official: [Understanding parameters](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Parameters/Understanding-ROS2-Parameters.html)
- Official: [About parameters](https://docs.ros.org/en/jazzy/Concepts/Basic/About-Parameters.html)
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
