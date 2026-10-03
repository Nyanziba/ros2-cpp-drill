# ROS 2 Lecture 12: Writing pub/sub in Python

## Introduction

After this article, you can write your own publisher and subscriber in Python with `rclpy`, build them, and run them. You will also connect the C++ node made in [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md) and the Python node on the same topic, and check that they communicate even though the languages are different.

The prerequisite is that you have read [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md) and [05_topics](05_topics.md). What you build is almost the same as the C++ version, so this article focuses on the differences from the C++ version.

## Lecture goals

- You can create a Python package with `ros2 pkg create --build-type ament_python`
- You can write a publisher/subscriber that inherits from `Node` of `rclpy`
- You can edit `entry_points` in `setup.py` so that the node can be run with `ros2 run`
- You can explain why `colcon build` is needed even for an `ament_python` package
- You can make C++ nodes and Python nodes communicate with each other over topics, and use either without worrying about the language difference

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy set up ([02_environment_setup](02_environment_setup.md) finished)
- The `cpp_pubsub` package (`talker`/`listener`) made in [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md) is already built
- The workspace `~/ros2_ws` exists, and you can put packages under `src`
- A screen where you can open about three terminals side by side

### Suggested time plan

- Creating the package and explaining the directory layout: 10 minutes
- Explaining the publisher/subscriber code: 20 minutes
- Build, run, and checking the sticking points: 15 minutes
- Exercise on communication with C++ nodes: 15 minutes
- Oral quiz: 10 minutes

### Oral quiz

**Q1. This is a package of the `ament_python` build type, so why is `colcon build` needed? Python is an interpreted language, so you should be able to run it directly.**

Model answer: `colcon build` is not a mechanism only for compiling. Even for an `ament_python` package, you need processing that generates the run scripts from `entry_points` in `setup.py`, places the package under the `install` directory, resolves the dependencies of `package.xml`, and registers it in the package search path of ROS 2 (`AMENT_PREFIX_PATH`). Without this registration, `ros2 run` cannot find the package. If you use `--symlink-install`, you can skip copying the source again, but the first `colcon build` itself is still required.

**Q2. What happens if you forget to edit `entry_points` in `setup.py`?**

Model answer: Even if you write the node file that `ros2 pkg create` generated, unless you add an entry to `console_scripts` in `entry_points`, the executable that can be run with `ros2 run <package name> <node name>` is not generated. `colcon build` succeeds, but `ros2 run` gives an error such as "No executable found". When you add a node, you must make it a habit to always edit `setup.py` too.

**Q3. Explain what to write in C++ and what to write in Python, from the viewpoint of the control period.**

Model answer: Processing that must run at hundreds of Hz to several kHz, such as motor control and sensor processing, and processing with a large amount of calculation, such as image processing and point cloud processing, should be written in C++. Python is not suited for high-frequency, low-latency processing because of the overhead of the interpreter and the effect of the Global Interpreter Lock (GIL). On the other hand, low-frequency logic such as high-level state transitions and task planning, tools for debugging, scripts for experiments, and processing that uses machine learning libraries are easier to write in Python, and development is faster. If you connect both with ROS 2 topics, you can write each part in the language it is good at.

## Main text

### What you learn: Create a Python package

What you learn: Create a package with the `ament_python` build type.

Preparation: Move to the `src` directory of the workspace.

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_python py_pubsub --dependencies rclpy std_msgs
```

In the C++ version we specified `ament_cmake` and `CMakeLists.txt` was generated, but this time it is `ament_python`, so the generated files are different.

```
py_pubsub/
├── package.xml
├── setup.py
├── setup.cfg
├── resource/
│   └── py_pubsub
├── py_pubsub/
│   └── __init__.py
└── test/
```

Note that there is no `CMakeLists.txt`, and there is a `setup.py` instead. A C++ `ament_cmake` package is built with CMake, but a Python `ament_python` package is built on the standard Python packaging mechanism (`setuptools`). The body of the node goes inside the `py_pubsub/py_pubsub/` directory, which has the same name as the package.

### What you learn: Write the publisher node

What you learn: Write a publisher that inherits from `Node` of `rclpy`.

Preparation: Create `publisher_member_function.py` in the `py_pubsub/py_pubsub/` directory.

Content:

This is the minimal publisher that follows the official tutorial.

```python
import rclpy
from rclpy.node import Node

from std_msgs.msg import String


class MinimalPublisher(Node):

    def __init__(self):
        super().__init__('minimal_publisher')
        self.publisher_ = self.create_publisher(String, 'topic', 10)
        timer_period = 0.5  # seconds
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.i = 0

    def timer_callback(self):
        msg = String()
        msg.data = 'Hello World: %d' % self.i
        self.publisher_.publish(msg)
        self.get_logger().info('Publishing: "%s"' % msg.data)
        self.i += 1


def main(args=None):
    rclpy.init(args=args)

    minimal_publisher = MinimalPublisher()

    rclpy.spin(minimal_publisher)

    # Destroy the node explicitly
    # (optional - otherwise it will be done automatically
    # when the garbage collector destroys the node object)
    minimal_publisher.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

You understand it faster if you compare it with `rclcpp::Node` in the C++ version.

| C++ (rclcpp) | Python (rclpy) |
|---|---|
| Inherit from `rclcpp::Node` | Inherit from `rclpy.node.Node` |
| `this->create_publisher<T>(...)` | `self.create_publisher(T, ...)` |
| `this->create_wall_timer(...)` | `self.create_timer(...)` |
| `RCLCPP_INFO(this->get_logger(), ...)` | `self.get_logger().info(...)` |
| `rclcpp::spin(node)` | `rclpy.spin(node)` |

The `timer_period` passed to `create_timer` is a float in seconds, and it is a 0.5 second period, the same as `std::chrono::milliseconds(500)` in the C++ version. `self.i` is a simple counter that increases by 1 each time the callback is called. It is put into the message so that you can follow "which publish this is" in the log.

At the end of the `main` function we call `destroy_node()`, but as the comment says, this is not required. It is called automatically when the garbage collector destroys the node object, so it works even if you do not write it. However, the official tutorial writes it explicitly to show that you can control the timing of resource release yourself, so you may leave it as it is.

### What you learn: Write the subscriber node

What you learn: Write a subscriber that inherits from `Node` of `rclpy`.

Preparation: Create `subscriber_member_function.py` in the same directory.

Content:

```python
import rclpy
from rclpy.node import Node

from std_msgs.msg import String


class MinimalSubscriber(Node):

    def __init__(self):
        super().__init__('minimal_subscriber')
        self.subscription = self.create_subscription(
            String,
            'topic',
            self.listener_callback,
            10)
        self.subscription  # prevent unused variable warning

    def listener_callback(self, msg):
        self.get_logger().info('I heard: "%s"' % msg.data)


def main(args=None):
    rclpy.init(args=args)

    minimal_subscriber = MinimalSubscriber()

    rclpy.spin(minimal_subscriber)

    # Destroy the node explicitly
    # (optional - otherwise it will be done automatically
    # when the garbage collector destroys the node object)
    minimal_subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

The third argument `listener_callback` of `create_subscription` is the function that is called when a message is received. In the C++ version we bound it with `std::bind` or a `[this](...)` lambda, but in Python you just pass the member function as it is, and it is handled inside as a method with the instance already bound. The line `self.subscription  # prevent unused variable warning` looks a little strange, but it is a charm to stop Python static analysis tools from wrongly flagging it as an "unused variable". In fact, the instance is kept for the lifetime of the node as soon as it is assigned to `self.subscription`, so this line itself does not affect the behavior.

### What you learn: Edit setup.py and package.xml

What you learn: Register the node in `entry_points` and make it runnable.

Preparation: Open `setup.py` in an editor.

Content:

The contents of `setup.py` look like the following (it shows the places you need to edit from the auto-generated state).

```python
from setuptools import find_packages, setup

package_name = 'py_pubsub'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='your_name',
    maintainer_email='you@example.com',
    description='Examples of minimal publisher/subscriber using rclpy',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'talker = py_pubsub.publisher_member_function:main',
            'listener = py_pubsub.subscriber_member_function:main',
        ],
    },
)
```

The most important part is `console_scripts` in `entry_points`. The notation `'talker = py_pubsub.publisher_member_function:main'` means "when you type `ros2 run py_pubsub talker`, run the `main` function of the `publisher_member_function` module of the `py_pubsub` package". Note that the names `talker` and `listener` written here are the same as the executable names (`talker` and `listener`) that you made in the C++ version. We intentionally use the same names in this article, but the package names (`cpp_pubsub`/`py_pubsub`) are different, so you can tell them apart when you run.

Rewrite `maintainer` and `maintainer_email` with your own information too (the build passes even if they are left empty, but a warning is shown).

On the `package.xml` side, we specified `--dependencies rclpy std_msgs` when running `ros2 pkg create`, so `exec_depend` was added automatically.

```xml
<exec_depend>rclpy</exec_depend>
<exec_depend>std_msgs</exec_depend>
```

In the C++ version you had to write `find_package` and `target_link_libraries` in `CMakeLists.txt`, but in Python the dependency resolution is complete with only `exec_depend` in `package.xml`. Since no compile is needed, the way to write dependencies is simpler than in the C++ version.

> Column: If you ran `ros2 pkg create` without `--dependencies`, no `<exec_depend>` is added to `package.xml`, so you need to add it by hand later. Forgetting to update `package.xml` when you add or remove a dependency is a point where ROS 2 beginners often get stuck, in both C++ and Python.

### What you learn: Build and run

What you learn: Build with `colcon build` and run with `ros2 run`.

Preparation: Move to the root of the workspace.

```bash
cd ~/ros2_ws
```

Content:

```bash
colcon build --packages-select py_pubsub
```

When the build is done, load the overlay in a new terminal and then run.

```bash
source install/setup.bash
ros2 run py_pubsub talker
```

In another terminal, load the overlay in the same way and then start the subscriber.

```bash
source install/setup.bash
ros2 run py_pubsub listener
```

If a log such as `Publishing: "Hello World: 0"` appears every 0.5 seconds on the `talker` side, and `I heard: "Hello World: 0"` appears at the same pace on the `listener` side, you succeeded.

**Why is `colcon build` needed even for `ament_python`?**

This is the point that is most often misunderstood in this lecture. You may think "Python is a language that needs no compile, so I should be able to `ros2 run` right after I write the file", but in fact you need to go through `colcon build`. There are three reasons.

1. You need to generate the executable scripts from `entry_points` in `setup.py` and place them in the `install` directory
2. You need to parse `package.xml`, verify the dependencies, and register the package in the ROS 2 package index (`share/ament_index/resource_index/packages`)
3. By loading `install/setup.bash`, the location of the package is added to `AMENT_PREFIX_PATH` and `PYTHONPATH`

In other words, `colcon build` is not a step for compiling. It is a step for registering the package in the package management system of ROS 2. Unlike C++, the build time is instant, but the step itself cannot be skipped.

**Sticking point: `--symlink-install`**

It is bad for development efficiency to run `colcon build` again each time, even when you fixed only one line of the source code. So we use the `--symlink-install` option.

```bash
colcon build --packages-select py_pubsub --symlink-install
```

With this option, symbolic links are made in the `install` directory instead of copying files. Python needs no compile, so if you edit the source file directly, the change is reflected as it is through the symbolic link, and it shows up in the result of `ros2 run` without a rebuild (if you changed `entry_points` in `setup.py` or added a new node, the links must be made again, so rebuild). C++ needs compiling, so even with this option you cannot skip the rebuild after changing the source. If you understand this difference, the development cycle of Python nodes gets one step faster.

Hint: If you add `--symlink-install` from the first build, you will not be in trouble later. If you already built without `--symlink-install`, it is safest to delete the `install` and `build` directories once and then build again with the option.

### What you learn: Connect C++ nodes and Python nodes

What you learn: Check that nodes written in different languages can communicate on the same topic.

Preparation: Check that the `cpp_pubsub` package from [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md) is already built.

Content:

Topics are connected only by the name and the message type, so the sender and the receiver do not need to be written in the same language. Let's check it for real.

In terminal 1, start the C++ talker.

```bash
ros2 run cpp_pubsub talker
```

In terminal 2, start the Python listener.

```bash
ros2 run py_pubsub listener
```

The Python listener should receive the messages sent by the C++ talker and show them in the log. Also check the opposite direction, that is, the combination of the Python talker and the C++ listener.

**Exercise**: Open terminal 3 additionally, and start the C++ talker, the Python talker, and the Python listener at the same time. The log of the `listener` should show the messages of both the C++ and the Python talker mixed together. Explain why this happens, based on how topics work.

<details markdown="1"><summary>Answer</summary>

A topic is not one-to-one communication. If the topic name and the message type are the same, many publishers and many subscribers can connect freely, so it is a many-to-many mechanism. The C++ talker and the Python talker both just publish to the `std_msgs/msg/String` topic named `topic`, and from the subscriber side there is no information to tell which language the sender process was written in. So the `listener` receives the messages from both publishers without distinguishing them, and the callback is called in the order they arrive. The middleware of ROS 2 (DDS) is the real body of inter-process communication, and it does not care about the language of the executable binary.

</details>

From this, you can see that in ROS 2 "which language to write in" is an implementation detail of a node, and in the system design you can mix them freely as long as the topic interface (name and type) matches.

**Which should you write in, C++ or Python?**

When you build a real project, you do not need to unify everything in C++ or everything in Python. The rough criteria are as follows.

| Nature of the processing | Suitable language | Reason |
|---|---|---|
| Processing that runs at hundreds of Hz or more, such as motor control and sensor drivers | C++ | The interpreter overhead and the GIL of Python become the bottleneck of low-latency processing |
| Processing with a large amount of calculation, such as image processing and point cloud processing | C++ | You need the execution speed of native code |
| Low-frequency processing centered on logic, such as state transitions and task planning | Python | Development is fast, and you can try more rewrites |
| Visualization, tools for debugging, small experiment scripts | Python | There are many libraries, and you can write it on the spot and throw it away |

It is not that you cannot write everything in C++, but "writing in C++ the places that do not need speed" gives little return for the effort of building. Conversely, "writing in Python the places that need speed" easily gets you stuck because of the GIL. Taking advantage of the property you checked this time, that both languages can be connected by topics, choosing the language according to the processing is what you can call a ROS 2 style design.

## Going further

Like `rclcpp` in C++, `rclpy` has a mechanism to run many callbacks concurrently with callback groups and a multi-threaded `Executor`. However, because of the GIL of Python, you do not benefit from concurrency as easily as in C++. Going deeper into this is beyond the scope of this article, so we leave it to another article.


## Wrapping up

Even with the same `talker`/`listener` structure, you probably felt that the amount of code and the feel of writing are quite different between the C++ version and the Python version. It is not that one is better. ROS 2 is designed on the premise that you choose between them according to the processing. First, let's become able to write both. If you do not understand something, ask someone experienced or check the official documentation.

Next is [13_custom_interfaces](13_custom_interfaces.md), which covers how to use a message type that you define yourself, instead of an existing type such as `std_msgs/msg/String`.

## References

- [ROS 2 Documentation: Jazzy — Writing a simple publisher and subscriber (Python)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Publisher-And-Subscriber.html)
- [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md)
- [05_topics](05_topics.md)
- [13_custom_interfaces](13_custom_interfaces.md)
