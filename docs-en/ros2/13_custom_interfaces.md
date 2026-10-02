# ROS 2 Lecture 13: Custom Interfaces

## Introduction

After this article, you understand the syntax of `.msg`/`.srv` files, and you can define your own message type, build it as a package, and use it from other nodes. You have already used topics and services, but the message types you used there were ready-made ones such as `geometry_msgs/msg/Twist`. This time you make such a type yourself.

The prerequisites are that you have finished up to [06_services](06_services.md), and that you can write Python pub/sub nodes with [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md).

## Lecture goals

- You can read and write the syntax of `.msg`/`.srv` (types, arrays, constants)
- You can make a custom interface package from scratch with `rosidl_default_generators`
- You learn the order "first look for a standard type, and make your own only if there is none"
- You can pub/sub your own msg, and check its structure with `ros2 interface show`

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco set up
- A workspace (such as `~/ros2_ws`, already created in [02_environment_setup](02_environment_setup.md))
- `sudo apt install ros-jazzy-rosidl-default-generators` is installed (normally already included if you installed `ros-jazzy-desktop`)

### Suggested time plan

- Criteria for looking for a standard type or making your own: 5 minutes
- Explaining the msg/srv syntax: 10 minutes
- Demo of creating the package (the equivalent of `tutorial_interfaces`): 15 minutes
- Exercise on pub/sub with your own msg: 15 minutes
- Talk about the link to CAN communication, and oral quiz: 10 minutes

### Oral quiz

**Q1. When you want to exchange new data, what should you do first?**

Model answer: Before you make your own msg, look in the existing standard message packages (`std_msgs`, `geometry_msgs`, `sensor_msgs`, and so on) with `ros2 interface list` or `ros2 interface package geometry_msgs` to see whether a matching type exists. The types that most robots in the world need are already standardized, such as `Twist` for velocity and `Pose` for pose. If you use a standard type, it connects as it is with other packages and other tools (such as `rviz2` and `tf2`), but your own type works only in your own package. Keep the order: if you look and there is none, make your own.

**Q2. Explain the difference between `int32[]`, `int32[3]`, and `int32[<=5]`.**

Model answer: `int32[]` is a variable-length array with no limit on the number of elements. `int32[3]` is a fixed-length array whose number of elements is always 3 (if it is not exactly 3, you get an error at generation or at run time). `int32[<=5]` is a variable-length array with an upper limit of 5 (0 to 5 elements). For data that always has all four wheels, such as encoder values, a fixed-length `float64[4]` is appropriate, and if you make it variable-length, the receiver has to write a check of the number of elements.

**Q3. Why is it common to separate the package that holds the `.msg` files from the package that uses them?**

Model answer: The message definition (the processing that automatically generates C++/Python code with `rosidl_generate_interfaces`) has special build dependencies, and if you split it out into a msg-only package, many node packages can reuse the same type just by calling `find_package`. If you mix logic and interface definitions in the same package, another package that wants only the types has to carry unnecessary dependencies. `tutorial_interfaces` in the official tutorial also has this separated structure.

## Main text

### What you learn: First look for a standard type

What you learn: Before you make your own, check whether the standard message types are enough.

Preparation: None.

Content:

ROS 2 already provides commonly used data structures as standard packages, such as `std_msgs`, `geometry_msgs`, `sensor_msgs`, and `nav_msgs`. For example, `geometry_msgs/msg/Twist` for velocity, `geometry_msgs/msg/Pose` for pose, and `sensor_msgs/msg/BatteryState` for the remaining battery.

To list what types a package has, use the following command.

```bash
ros2 interface package geometry_msgs
```

When you want to search for a type among all installed packages, passing `ros2 interface list` through `grep` is quick.

```bash
ros2 interface list | grep -i battery
```

**If a standard type can express it, use the standard type.** Your own type does not work outside your own package, so when you want to work with existing tools such as `rviz2` and `tf2`, first think about whether you can express it with a standard type. Only when data appears that you still cannot express (such as sensor values unique to your project, or fields of your own protocol), make your own with the steps below.

### What you learn: The syntax of msg/srv files

What you learn: Understand how to write types, arrays, and constants in `.msg`/`.srv`.

Preparation: None.

Content:

A `.msg` file is a simple text file where you just write one field per line in the order "type name".

```
# Num1.msg
int64 num
```

You line up many fields in the same way.

```
# AddThreeInts.srv (for a service. The request and the response are separated by ---)
int64 a
int64 b
int64 c
---
int64 sum
```

The types you can use are basic types such as `int8`/`int32`/`int64`/`float32`/`float64`/`string`/`bool`, and you can also use message types of other packages as they are.

```
# Example of embedding another msg as a field
geometry_msgs/Vector3 position
string name
```

There are three ways to write arrays.

| How to write | Meaning |
|---|---|
| `int32[]` | Variable-length array (no limit on the number of elements) |
| `int32[3]` | Fixed-length array (always 3 elements) |
| `int32[<=5]` | Variable-length array with an upper limit (0 to 5 elements) |

You define constants in capital letters, and use them as fields whose value cannot be changed. They are often used to express choices decided in advance, such as status values.

```
# Example of constants
int32 STATUS_IDLE=0
int32 STATUS_RUNNING=1
int32 STATUS_ERROR=2
int32 status
```

You can write comments with `#`. If you state the unit of a field (such as `# rad/s`) in a comment, the person who uses it later is not confused. **If you forget to write the unit, the person who looks later cannot tell whether it is `rad`, `deg`, or `rad/s`, and an accident happens**, so do not skip this.

### What you learn: Create a custom interface package

What you learn: Create from scratch a package that can build `.msg`/`.srv` with `rosidl_default_generators`.

Preparation: Check that you are in `src` of the workspace.

```bash
cd ~/ros2_ws/src
```

Content:

We make a package equivalent to `tutorial_interfaces` of the official tutorial. It is a package only for interfaces that can be used from both C++ and Python nodes, so the build type is only `ament_cmake` (even when you use it from Python nodes, the msg/srv themselves are built with CMake).

```bash
ros2 pkg create --build-type ament_cmake tutorial_interfaces
```

Inside the created package, make a `msg` directory and a `srv` directory, and put a file in each. `ros2 pkg create` does not change the current directory, so do not forget to **move into the package first**. If you forget, `msg`/`srv` are made directly under `src/`, and `rosidl_generate_interfaces`, described later, cannot find the files and the build breaks.

```bash
cd tutorial_interfaces
mkdir msg srv
```

Create `msg/Num.msg`.

```
int64 num
```

Also create `srv/AddThreeInts.srv`.

```
int64 a
int64 b
int64 c
---
int64 sum
```

Next, add the dependencies to `package.xml`. Put the following three lines near below `<buildtool_depend>`.

```xml
<buildtool_depend>rosidl_default_generators</buildtool_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

Finally, edit `CMakeLists.txt`. Add the following below `find_package(ament_cmake REQUIRED)`.

```cmake
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Num.msg"
  "srv/AddThreeInts.srv"
)
```

`rosidl_generate_interfaces` is the build step that automatically generates the C++/Python type definition code from `.msg`/`.srv` files. The file paths you write here become the build targets as they are, so when you add a new msg, always add it to this list too (**if you forget, the file exists but is not built, and you get a "type not found" error**, a common trap).

Go back to the root of the workspace and build.

```bash
cd ~/ros2_ws
colcon build --packages-select tutorial_interfaces
source install/setup.bash
```

When the build passes, you can check your own type with `ros2 interface show`.

```bash
ros2 interface show tutorial_interfaces/msg/Num
```

```
int64 num
```

### What you learn: Use your own msg from another package

What you learn: Refer to your own type from a node package and pub/sub it.

Preparation: The build of `tutorial_interfaces` is finished. Use the package made in [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md) (or a package newly created for this lecture).

Content:

Add the dependency to `package.xml` of the package that uses your own msg.

```xml
<exec_depend>tutorial_interfaces</exec_depend>
```

The work of adding the dependency to `CMakeLists.txt` (for C++) or `setup.py` (for Python) is exactly the same as adding a dependency on a standard message package (such as `std_msgs`). There is nothing special.

The import in a Python node is also as usual.

```python
from tutorial_interfaces.msg import Num
```

In the publisher, you only replace the place that used `std_msgs/msg/Int64` with `Num`.

```python
msg = Num()
msg.num = self.count
self.publisher_.publish(msg)
```

**Exercise**: Create the `tutorial_interfaces` package that has `msg/Num.msg`, and write a publisher node that publishes a `Num` that counts up every second, and a subscriber node that prints the received value to the log. Check with `ros2 topic echo` that the value is flowing.

<details markdown="1"><summary>Answer</summary>

Creating the package and building it follow the steps in the main text. The skeleton of the publisher node is as follows.

```python
import rclpy
from rclpy.node import Node
from tutorial_interfaces.msg import Num


class NumPublisher(Node):
    def __init__(self):
        super().__init__('num_publisher')
        self.publisher_ = self.create_publisher(Num, 'topic', 10)
        self.timer_ = self.create_timer(1.0, self.timer_callback)
        self.count_ = 0

    def timer_callback(self):
        msg = Num()
        msg.num = self.count_
        self.publisher_.publish(msg)
        self.get_logger().info(f'Publishing: {msg.num}')
        self.count_ += 1


def main(args=None):
    rclpy.init(args=args)
    node = NumPublisher()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
```

For the subscriber too, you just write `create_subscription(Num, 'topic', callback, 10)`, and the rest is no different from the usual way of writing pub/sub. If you can see `num: 0`, `num: 1` increasing with `ros2 topic echo /topic`, you succeeded.

</details>

### What you learn: The meaning of custom interfaces in robot competitions

What you learn: Gain a design viewpoint on how to map the contents of CAN communication with the microcontroller to msgs on the ROS side.

Preparation: None.

Content:

You really need your own msg when you communicate with the microcontroller side. Our robot exchanges motor commands and encoder values with the microcontroller over CAN, but a CAN frame is a byte string, and it is not a ROS 2 topic in itself. The bridge node on the CAN side needs to parse the received byte string and convert it again into a ROS 2 msg that has fields with meaning.

Here, the basic rule is to move what a standard type can express toward the standard type. For a velocity command, `geometry_msgs/msg/Twist` is often enough, and for a simple ON/OFF, `std_msgs/msg/Bool` is often enough. On the other hand, for data such as raw encoder values, or several sensor values packed in each CAN frame ID, it is easier to read if you express it plainly with your own msg than if you force it into a standard type.

```
# Example of your own msg: assumed to pass the encoder values of the motors as they are
# (Match the real field layout to the definition of your own CAN protocol. This is only an example)
float64[4] encoder_position
float64[4] encoder_velocity
bool[4] motor_enabled
```

Which CAN frame and which bit position map to which msg field must follow the protocol specification of CAN communication that your project has decided. If you write a made-up mapping table here, it will differ from the real one and cause an accident, so always check the primary source.


## Going further

Some projects do not put many msg files into one package, and manage them split by function, such as `base_interfaces`. The more packages you have, the more complex the dependency management becomes, so how far to split depends on how you operate.

Also, not only `.msg` but also `.action` (an interface for actions) can be generated with the same `rosidl_generate_interfaces` mechanism. Actions themselves are covered in [08_actions](08_actions.md), but how to make your own action type is not covered there.


## Wrapping up

Make it a habit to check first whether a standard type is enough. Your own msgs are convenient, but if you add too many, you cannot maintain the dependencies and the documentation. If you do not understand something, ask someone experienced or check the official documentation.

Next is [14_implementing_services](14_implementing_services.md), where you write a service node that actually uses the `AddThreeInts.srv` made this time.

### Matching exercise

After you read this chapter, practice with the matching drill.

- `03_custom_interface` — Define and use a custom interface

```bash
./drill run 03
```

From the exercise, you can return to this chapter with `./drill read`.

## References

- [ROS 2 Documentation: Jazzy — Creating custom msg and srv files](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.html)
- [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md)
- [14_implementing_services](14_implementing_services.md)
