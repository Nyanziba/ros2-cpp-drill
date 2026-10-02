# Exercise 09: Write launch files in Python / XML / YAML [Advanced]

As the official guide
[Using Python, XML, and YAML for ROS 2 launch files](https://docs.ros.org/en/jazzy/How-To-Guides/Launch-file-different-formats.html)
says, you can write a ROS 2 launch file in Python, XML, or YAML.
Even if the format differs, the "list of nodes to launch" (`LaunchDescription`) that is built internally
is the same.

This exercise is **graded differently from the others**. It is graded by `pytest`, not by C++.
It actually loads the three files `launch/talker_listener.launch.py` / `.xml` / `.yaml`
and compares their structure (package / executable / namespace / remap).

## What to do

In the three files in the `launch/` directory, write the **same content** in three formats.
Launch the `talker` of exercise 01 (`drill_01_publisher`) and the
`listener` of exercise 02 (`drill_02_subscriber`) with the following settings.

| Item | talker | listener |
| --- | --- | --- |
| package | `drill_01_publisher` | `drill_02_subscriber` |
| executable | `talker` | `listener` |
| name | `talker` | `listener` |
| namespace | `demo` | `demo` |
| remap | `topic` → `chatter` | `topic` → `chatter` |

- `launch/talker_listener.launch.py`
- `launch/talker_listener.launch.xml`
- `launch/talker_listener.launch.yaml`

In all three files, the comments in the file give the skeleton of the element names. Fill in the values.
When you have filled in all three, delete the `I AM NOT DONE` line in each file.

## Comparing the three formats and when to use each

| Aspect | Python | XML | YAML |
| --- | --- | --- | --- |
| Readability | Easy to read if you are used to programming, but gets long vertically as it grows | The nesting of tags makes the structure easy to see. Close to ROS 1 launch files | The indentation makes the structure easy to see. The shortest to write |
| Conditions and loops | You can use `if` / `for` as they are. You can write any Python code | There are `IfCondition` / `UnlessCondition`, but no loops | Like XML, there are conditions (`if`/`unless` attributes) but no loops |
| Including other files | You can mix any logic into `IncludeLaunchDescription` (for example, change the file to read depending on a condition) | You can write it declaratively with `<include file="...">` | You can write it declaratively with `include:` |
| Completion support | It is just Python, so IDE completion and type checking work as they are | Without schema completion, you cannot write it unless you remember the attribute names | Like XML, you cannot write it unless you remember them |

**Rule of thumb**: For a static setup (the list of nodes to launch is fixed and the values are hard-coded),
XML or YAML is shorter and easier to read. If you need conditions, calculations, or a dynamically changing number of nodes,
Python is the only choice (once you need `if` or `for`, XML/YAML starts to strain).
In real work, a mix is also common: "the big picture in XML/YAML, and only the complex parts built in Python with `OpaqueFunction`".

## How to write commonly used elements

`DeclareLaunchArgument` (declares an argument that can be passed at launch time):

| Format | How to write it |
| --- | --- |
| Python | `DeclareLaunchArgument("use_sim_time", default_value="false")` |
| XML | `<arg name="use_sim_time" default="false"/>` |
| YAML | `- arg: {name: use_sim_time, default: "false"}` |

`LaunchConfiguration` (refers to the value of a declared argument):

| Format | How to write it |
| --- | --- |
| Python | Pass `LaunchConfiguration("use_sim_time")` to something like `Node(parameters=[{...}])` |
| XML | Write `$(var use_sim_time)` inside an attribute value |
| YAML | Write `$(var use_sim_time)` inside an attribute value (the same syntax as XML) |

`IncludeLaunchDescription` (loads another launch file):

| Format | How to write it |
| --- | --- |
| Python | `IncludeLaunchDescription(PythonLaunchDescriptionSource([FindPackageShare("pkg"), "/launch/other.launch.py"]))` |
| XML | `<include file="$(find-pkg-share pkg)/launch/other.launch.py"/>` |
| YAML | `- include: {file: "$(find-pkg-share pkg)/launch/other.launch.py"}` |

`GroupAction` + `PushRosNamespace` (puts several nodes into one namespace together):

| Format | How to write it |
| --- | --- |
| Python | `GroupAction([PushRosNamespace("demo"), Node(...), Node(...)])` |
| XML | `<group> <push_ros_namespace namespace="demo"/> <node .../> <node .../> </group>` |
| YAML | `- group: {children: [{push_ros_namespace: {namespace: demo}}, {node: {...}}, {node: {...}}]}` |

In this exercise, `namespace="demo"` is given directly on each `Node`, but
as the number of nodes grows, grouping them with `GroupAction` + `PushRosNamespace`
reduces writing mistakes.

## Run it

**For the manual check, exercise 01 (`drill_01_publisher`) and exercise 02 (`drill_02_subscriber`) must be
implemented and already built with `colcon build`.** Even if the launch files themselves are
unfinished, `colcon build --packages-select drill_09_launch` passes,
but to actually start the nodes you need the executables of 01 and 02.

```bash
colcon build --packages-select drill_01_publisher drill_02_subscriber drill_09_launch
source install/setup.bash

ros2 launch drill_09_launch talker_listener.launch.py
# it should start the same way with .xml / .yaml too
# ros2 launch drill_09_launch talker_listener.launch.xml
# ros2 launch drill_09_launch talker_listener.launch.yaml
```

In another terminal:

```bash
ros2 node list
# /demo/talker
# /demo/listener

ros2 topic list
# you should see /demo/chatter (if the remap works, /demo/topic does not appear)
```

## Common pitfalls

- `remap` in XML / YAML is nested **inside** `<node>` / `node:`.
  If you put it outside the `<node>` tag, or at the same level as `node:`, it is not recognized as an attribute.
- `namespace` adds `/demo/` in front of a topic name such as `topic`.
  It has a different role from `name` (the node name), so do not confuse them.
- In YAML, the indentation is the structure itself. If you mix in tab characters, you get a load error.
- If you forget to delete `I AM NOT DONE` in any of the three files, `./drill list` still shows the exercise as not done.

## Tests

```bash
./drill run 09
```

| Test | What it checks |
| --- | --- |
| `test_python_launch_starts_talker_and_listener` | Whether the Python version has two `Node`s (with the correct package/executable) |
| `test_python_launch_has_demo_namespace_and_chatter_remap` | The namespace and remap values of the Python version |
| `test_xml_launch_matches_python_structure` | Whether the XML version has the same structure as the Python version |
| `test_yaml_launch_matches_python_structure` | Whether the YAML version has the same structure as the Python version |
| `test_all_three_formats_are_equivalent` | Whether the structure of all three formats matches exactly |

The tests do not actually start node processes. They use the API of `launch` / `launch_ros`
to load the launch files, and extract only the values of `package` / `executable` / `namespace` /
`remap` as strings to compare them.

## References

- Official: [Creating a launch file](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Launch/Creating-Launch-Files.html)
- Official: [Using Python, XML, and YAML for ROS 2 launch files](https://docs.ros.org/en/jazzy/How-To-Guides/Launch-file-different-formats.html)
- Source: `/opt/ros/jazzy/lib/python3.12/site-packages/launch/frontend/parser.py`
- Source: `/opt/ros/jazzy/lib/python3.12/site-packages/launch_ros/actions/node.py`
