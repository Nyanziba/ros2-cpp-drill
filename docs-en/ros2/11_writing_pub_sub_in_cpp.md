# ROS 2 Lecture 11: Writing pub/sub in C++

## Introduction

After this article, you can write your own publisher node and subscriber node in C++, build them, and make them actually communicate.

The prerequisite is that you have read [10_workspaces_and_colcon](10_workspaces_and_colcon.md). If you do not know how to make a workspace and the flow of colcon build, you will get stuck at the package creation in this article.

## Lecture goals

- You can create a C++ package with `ros2 pkg create`
- You can write a publisher node and a subscriber node that inherit from `rclcpp::Node`
- You can add the dependencies correctly to `package.xml` and `CMakeLists.txt`
- You can run the built nodes in two terminals and check the communication with `ros2 topic echo`

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco set up ([02_environment_setup](02_environment_setup.md) finished)
- A workspace (for example `~/ros2_ws`) is already created, and `colcon build` has passed at least once ([10_workspaces_and_colcon](10_workspaces_and_colcon.md) finished)
- An editor (such as VS Code) with C++ syntax highlighting working
- A screen where you can open three terminals side by side (for building, for running the publisher, and for running the subscriber)

### Suggested time plan

- Creating the package and explaining the directory layout: 10 minutes
- Reading the publisher/subscriber code: 20 minutes
- Editing CMakeLists.txt/package.xml and building: 15 minutes
- Running, checking with echo, and fixing sticking points: 15 minutes
- Oral quiz: 10 minutes

### Oral quiz

**Q1. What decides when the callback function passed to `create_wall_timer` is called? How is it different from receiving a topic?**

Model answer: `create_wall_timer` calls the callback automatically at the given period, based on the elapsed time of the wall clock. This has nothing to do with receiving topics, and it fires at a fixed interval whether a message comes or not. On the other hand, the callback of a subscriber is called when a message is actually published to the topic, so it is event-driven. Being able to leave the publisher's loop to a timer is the ROS 2 way of writing, and you do not need to write your own loop such as `while(true)`.

**Q2. What happens if you do not call `rclcpp::spin(node)`?**

Model answer: `spin` is the function that runs the node's event loop, and its job is to actually call the callbacks of timers and subscribers. If you do not call `spin`, the node starts and the program ends right away, and the callback is never run. For both the publisher and the subscriber, just registering a timer or a subscriber in the constructor does nothing, and they start to work only when `spin` is waiting for events.

**Q3. What kind of error do you get if you forget to write `ament_target_dependencies` in CMakeLists.txt?**

Model answer: The compile itself may go on as long as the `#include` works, but you often get a linker error such as `undefined reference to rclcpp::...` at link time. `ament_target_dependencies` has the job of attaching the include paths and libraries of the package (`rclcpp` or `std_msgs`) to the target, so if you forget it, the header is found but the symbols cannot be resolved. Conversely, if you forget the dependency declaration in `package.xml`, when you try to build this package in another environment, the dependency packages are not installed automatically, and it also affects how colcon resolves the build order.

## Main text

### What you learn: Create a package

What you learn: Create a new package for C++ with `ros2 pkg create`.

Preparation: Move to the `src` directory of the workspace.

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake --license Apache-2.0 cpp_pubsub
```

Content:

`--build-type ament_cmake` is the option that says this is a C++ package. For a Python package you use `ament_python`, which is covered in [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md). When you run it, the following layout is generated.

```
cpp_pubsub/
├── CMakeLists.txt
├── include/
│   └── cpp_pubsub/
├── package.xml
└── src/
```

You put source files in `src/`, write the build settings in `CMakeLists.txt`, and write the package metadata and dependencies in `package.xml`. If you do not edit these two files correctly, the build does not pass however correct the code is, so keep this layout in your head before you write code.

### What you learn: Write the publisher node

What you learn: Write a node that publishes a message at a fixed period, following the code of the official tutorial.

Preparation: Create `cpp_pubsub/src/publisher_member_function.cpp`.

Content:

The following is the publisher code of the [official tutorial](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html) with explanatory comments added. First, here is the whole code.

```cpp
#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

// Make the std::chrono literals (such as 500ms) available
using namespace std::chrono_literals;

// Make our own node class by inheriting from rclcpp::Node
class MinimalPublisher : public rclcpp::Node
{
public:
  MinimalPublisher()
  : Node("minimal_publisher"), count_(0)
  {
    // Publish String messages to the topic named "topic"
    // The 10 in the second argument is the QoS depth (the depth of the send queue)
    publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);

    // Create a timer that calls timer_callback every 500ms
    timer_ = this->create_wall_timer(
      500ms, std::bind(&MinimalPublisher::timer_callback, this));
  }

private:
  void timer_callback()
  {
    auto message = std_msgs::msg::String();
    message.data = "Hello, world! " + std::to_string(count_++);
    RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
    publisher_->publish(message);
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  size_t count_;
};

int main(int argc, char * argv[])
{
  // Initialize rclcpp (always call this before creating a node)
  rclcpp::init(argc, argv);

  // Create the node and pass it to spin. spin keeps calling the callbacks
  rclcpp::spin(std::make_shared<MinimalPublisher>());

  // When we leave with Ctrl+C or similar, do the shutdown
  rclcpp::shutdown();
  return 0;
}
```

There are three key points.

- **Timer-callback driven**: This publisher does not wait for some event. The `timer_callback` is called regularly by the 500ms timer registered with `create_wall_timer`. You do not need to write a loop yourself, and you only give the period once in the constructor.
- **QoS depth 10**: The second argument `10` of `create_publisher` is a QoS setting that says how many messages the send-side queue keeps. Even if the subscriber's receive processing is delayed for a moment, the latest 10 messages are buffered and are less likely to be lost. For the deeper QoS topic, see [05_topics](05_topics.md).
- **`std::bind`**: `std::bind(&MinimalPublisher::timer_callback, this)` converts the member function `timer_callback` into a callable object bound to `this` (the current instance). Writing `[this]() { this->timer_callback(); }` as a lambda has the same meaning. What the official Jazzy tutorial actually makes the reader write is the lambda version (`publisher_lambda_function.cpp` / `subscriber_lambda_function.cpp`), and the member function version with `std::bind` used here is listed as another variant in the `ros2/examples` repository. The ROS 2 style guide ([Code style and language versions](https://docs.ros.org/en/jazzy/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html)) clearly says "no restriction on lambdas, `std::function`, and `std::bind`", so you may use either. By the way, what the official documentation clearly discourages is not a way of writing but "the way of writing that does not inherit from `rclcpp::Node`" (because it cannot be made into a component).

### What you learn: Write the subscriber node

What you learn: Write a node that receives a topic and prints it to the log.

Preparation: Create `cpp_pubsub/src/subscriber_member_function.cpp`.

Content:

```cpp
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using std::placeholders::_1;

class MinimalSubscriber : public rclcpp::Node
{
public:
  MinimalSubscriber()
  : Node("minimal_subscriber")
  {
    // Subscribe to the topic named "topic"
    // The 10 in the second argument is the QoS depth, same as on the publisher side
    subscription_ = this->create_subscription<std_msgs::msg::String>(
      "topic", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));
  }

private:
  void topic_callback(const std_msgs::msg::String & msg) const
  {
    RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg.data.c_str());
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MinimalSubscriber>());
  rclcpp::shutdown();
  return 0;
}
```

Notice the symmetry with the publisher. `create_publisher` becomes `create_subscription`, and instead of a timer we pass the callback function `topic_callback` directly. The `_1` of `std::bind` is a placeholder that shows the position of the argument through which the message is passed, and `topic_callback(msg)` is called each time a message actually arrives. `topic_callback` is a `const` member function because it only prints the received message to the log and does not change the state of the node itself.

The topic name `"topic"` and the QoS depth `10` of the subscriber must match the publisher side. If you change only one side, the communication does not connect, so be careful.

### What you learn: Edit CMakeLists.txt and package.xml

What you learn: Write the dependencies and the executable settings needed for the build.

Preparation: Open `cpp_pubsub/CMakeLists.txt` and `cpp_pubsub/package.xml`.

Content:

First, add the dependency packages to `package.xml`. Near below `<description>`, where the existing `<depend>` tags are lined up, add the following.

```xml
<depend>rclcpp</depend>
<depend>std_msgs</depend>
```

Next is `CMakeLists.txt`. Below the `find_package(ament_cmake REQUIRED)` that is generated by default, add `find_package` for the packages you use.

```cmake
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
```

After that, add the definitions of the executables. For each node you need a pair of `add_executable` and `ament_target_dependencies`.

```cmake
add_executable(talker src/publisher_member_function.cpp)
ament_target_dependencies(talker rclcpp std_msgs)

add_executable(listener src/subscriber_member_function.cpp)
ament_target_dependencies(listener rclcpp std_msgs)

install(TARGETS
  talker
  listener
  DESTINATION lib/${PROJECT_NAME})
```

`add_executable` means "make an executable with this name from this source file", and `ament_target_dependencies` means "attach the headers and libraries of this package to this target". If you do not write `install`, then even if `colcon build` passes, the executables are not placed at the install destination (under `ros2_ws/install/`), and you end up with `ros2 run` not finding them.

### What you learn: Build and run

What you learn: Build with `colcon build` and run the publisher and the subscriber in two terminals.

Preparation: Move to the root of the workspace.

Content:

```bash
cd ~/ros2_ws
colcon build --packages-select cpp_pubsub
source install/setup.bash
```

Narrowing the target with `--packages-select` makes the build faster. When the build passes, prepare two terminals, run `source install/setup.bash` in each, and then run.

Terminal 1 (publisher):

```bash
ros2 run cpp_pubsub talker
```

Terminal 2 (subscriber):

```bash
ros2 run cpp_pubsub listener
```

If a log such as `Publishing: 'Hello, world! 0'` appears every 500ms on the talker side, and the matching `I heard: 'Hello, world! 0'` appears on the listener side, you succeeded. If you run `ros2 topic echo /topic` from a third terminal, you can also check that the same data is flowing.

**Exercise**: Check that the communication works correctly even if you start the listener after the talker is running. Then predict what happens if you start the listener first, and try it.

<details markdown="1"><summary>Answer</summary>

The communication works in either order. This is because the topics of ROS 2 find each other and connect, regardless of the order in which nodes start, thanks to the discovery feature of DDS (for the details of discovery, see the Going further section of [05_topics](05_topics.md)). However, if you start the listener later, it cannot receive the messages that were published before it started. The buffer of the QoS depth is a short-term measure against missed messages, assuming that the publisher and the subscriber are connected, and it does not go back and deliver the messages from the time they were not connected.

</details>

### Common pitfalls

- **Forgetting to edit CMakeLists.txt**: Even if you add a source file, if you do not write `add_executable`, `colcon build` passes without any problem (it simply does not know the new node exists). If `ros2 run cpp_pubsub talker` shows an error of the `No executable found` kind, not `Package 'cpp_pubsub' not found`, first check `add_executable` and `install` in CMakeLists.txt.
- **Forgetting to add a dependency**: If you forget `<depend>rclcpp</depend>` in `package.xml`, it is hard to notice while you build it alone, but it becomes an error where `rosdep` resolves dependencies and in CI. Even if only the CMakeLists.txt side has `find_package`, a mismatch with `package.xml` can show a warning in `colcon build`, so do not ignore warnings.
- **Forgetting to write `ament_target_dependencies`**: The build error appears as a linker error such as ``undefined reference to `rclcpp::...` ``. When "the compile passed but the link failed", suspect this first. If you remember that `#include` of a header and linking a library are separate settings, you will not panic at this kind of error.
- **Forgetting `source install/setup.bash`**: In a new terminal right after you rebuilt, the location of the new executable is not yet reflected in that terminal. If a node shows the old behavior even though you changed it, first check whether you forgot `source`.

## Going further

Once the talker and the listener work, modify an existing node and write a node that subscribes to `cmd_vel` and prints its content to the log. It is a modification that only receives a topic of type `geometry_msgs/msg/Twist` and shows `linear.x` and `angular.z` with `RCLCPP_INFO`. You need to replace `std_msgs::msg::String` with `geometry_msgs::msg::Twist`, and add the dependency on `geometry_msgs` to `package.xml` and `CMakeLists.txt`. Combine it with `turtlesim` from [05_topics](05_topics.md), and if your own node can receive the velocity commands of the turtle moved by `turtle_teleop_key`, you succeeded.

## Wrapping up

Pub/sub in C++ is the most basic form for writing a ROS 2 node. The structure of the talker/listener you wrote here (inheriting from Node, create_publisher/create_subscription, registering in the constructor, running with spin) appears again and again in any node you write from now on. If you do not understand something, ask a senior student.

Next is [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md), where you write the same thing in Python. Compare what is different and what is the same as C++.

### Matching exercise

After you read this chapter, practice with the matching drill.

- `01_publisher` — Publish to a topic
- `02_subscriber` — Subscribe to a topic

```bash
./drill run 01
./drill run 02
```

From the exercise, you can return to this chapter with `./drill read`.

## References

- [ROS 2 Documentation: Jazzy — Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
- [10_workspaces_and_colcon](10_workspaces_and_colcon.md)
- [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md)
