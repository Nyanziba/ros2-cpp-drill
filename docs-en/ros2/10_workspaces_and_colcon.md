# ROS 2 Lecture 10: Workspaces and colcon

## Introduction

After this article, you understand the structure of a ROS 2 workspace, you can build a package with `colcon`, and you can run a node that you built yourself.

The prerequisite is that you have read up to [09_launch_and_ros2_bag](09_launch_and_ros2_bag.md). So far you have used ready-made packages installed with apt (such as turtlesim), but from here on there will be more cases where you build and use code written by you or your team. The workspace is the base for that.

## Lecture goals

- You can explain the roles of `src`/`build`/`install`/`log` in a workspace
- You can build a package with `colcon build`, and you can choose between `--symlink-install` and `--packages-select`
- You understand the relationship between underlay and overlay, and you do not get the order of sourcing wrong
- You can resolve dependency packages with `rosdep`

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco set up ([02_environment_setup](02_environment_setup.md) finished)
- `python3-colcon-common-extensions` (if not installed, run `sudo apt install python3-colcon-common-extensions`)
- `python3-rosdep` (`sudo apt install python3-rosdep`. Only the first time, you need `sudo rosdep init && rosdep update`)
- A network environment where you can clone with git

### Suggested time plan

- Explaining the workspace structure: 10 minutes
- Exercise from creating a workspace to colcon build: 15 minutes
- underlay/overlay and the order of sourcing: 10 minutes
- Building and running the examples repository: 20 minutes
- Oral quiz: 5 minutes

### Oral quiz

**Q1. What are the `build`, `install`, and `log` directories that appear right after you run `colcon build`?**

Model answer: `build` is the working directory for intermediate files made during the build (such as the CMake cache and object files). `install` is the "place for finished products", where the executables, libraries, `setup.bash`, and so on that you actually use are placed. `log` holds the build logs, and when you look for the cause of an error, you look under `log/latest_build/`. Only `install` is needed at run time, and `build` and `log` can be restored by building again even if you delete them.

**Q2. The build has become strange, so you want to delete the `build` directory. What should you be careful about?**

Model answer: It is safe to delete `install` together with `build`, and then run `colcon build` again. If you delete only part of it, for example when the CMake cache remains only in `build`, the contents of the old `install` and the new build may not match, and you get an error with an unknown cause. When in doubt, the fastest way is `rm -rf build install log` and a clean build. However, all other packages are also built again, so do it when you have time.

**Q3. When you open a new terminal, what should you source first? Answer including the order.**

Model answer: First source `/opt/ros/jazzy/setup.bash` (the underlay, ROS 2 itself), and after that source `install/setup.bash` of your workspace (the overlay). The `setup.bash` on the overlay side assumes that the underlay is already loaded, so if you reverse the order, you get errors, or the commands of ROS 2 itself cannot be found. The common way is to write the sourcing of the underlay in `~/.bashrc`, and to source the overlay of a workspace by hand each time you use it.

## Main text

### What you learn: What is a workspace?

What you learn: The structure of a ROS 2 workspace.

Preparation: None.

Content:

A workspace is a working place for building many ROS 2 packages together. You rarely build a single package alone. The basic way to use it is to pass a group of packages that depend on each other to `colcon` together.

First, create a workspace.

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws
```

You only need to remember this directory. `colcon build` looks at the contents of `src`, and the other `build`, `install`, and `log` are created automatically in place when you run it.

| Directory | Role |
|---|---|
| `src` | The place for the source code of packages. This is the only place where you clone with git or write files |
| `build` | Intermediate files of the build. The CMake cache and object files |
| `install` | The finished products of the build. Executables, libraries, `setup.bash` |
| `log` | Build logs. You look at them when you trace the cause of an error |

You only touch `src`. The other three are areas managed by `colcon`, so you do not need to edit their contents by hand. Conversely, `build`, `install`, and `log` come back if you build again even after you delete them, so they are areas that you do not need to be afraid to delete.

### What you learn: The basics of colcon build

What you learn: Build a workspace with `colcon build`.

Preparation: `~/ros2_ws/src` exists.

Content:

Even when there is no package in `src`, `colcon build` creates the initial state of the workspace (empty `build`/`install`/`log`).

```bash
cd ~/ros2_ws
colcon build
```

The form you use every time in practice has the following option.

```bash
colcon build --symlink-install
```

With `--symlink-install`, many files under `install` become symbolic links instead of real copies. When you edit files that do not need compilation, unlike C++, such as Python nodes, launch files, and parameter yaml files, this form reflects the change right away without building again. The C++ executable itself still needs to be recompiled, but this option reduces rework for the other parts, so build with this option during development as a rule.

```bash
colcon build --packages-select <package name>
```

When a workspace has many packages, sometimes you want to build only the package you changed. `--packages-select` is an option that narrows the build targets, and it is handy when the number of packages grows and the build time gets long. If you want to build the packages that it depends on too, use `--packages-up-to`, but for now it is enough to remember only `--packages-select`.

After the build finishes, check the finished products with `ls install`. You should see directories named after the packages.

### What you learn: Underlay, overlay, and the order of sourcing

What you learn: The relationship between ROS 2 itself (the underlay) and your own workspace (the overlay).

Preparation: `colcon build` has succeeded at least once.

Content:

Here we sort out the meaning of the following command, which you typed without thinking each time you opened a new terminal in the previous articles.

```bash
source /opt/ros/jazzy/setup.bash
```

This is the command that enables ROS 2 itself (the part installed with apt). We call this base the underlay. To enable the workspace that you built with `colcon build`, you source `install/setup.bash`.

```bash
source ~/ros2_ws/install/setup.bash
```

We call this workspace side the overlay. The overlay is something you "lay over" the underlay, so the order of sourcing is always underlay → overlay. If you reverse it, the environment variables of ROS 2 itself that the overlay's `setup.bash` assumes (such as `AMENT_PREFIX_PATH`) are not set, and then the `ros2` command itself cannot be found, or you get an error saying a package cannot be found.

```bash
# Correct order
source /opt/ros/jazzy/setup.bash
source ~/ros2_ws/install/setup.bash

# Wrong (the overlay is sourced first)
source ~/ros2_ws/install/setup.bash
source /opt/ros/jazzy/setup.bash
```

If you write the underlay in `~/.bashrc`, it is sourced automatically in new terminals, but for the overlay the basic way is to type it by hand each time you use it. When you use several workspaces, writing the overlay in `.bashrc` makes it hard to tell "which workspace is active", so I recommend typing it by hand on purpose.

Another point where people get stuck is **mixing building and sourcing in the same terminal**. If you run `source install/setup.bash` right after `colcon build` in the same terminal, it seems to work. But when you run `colcon build` again in that terminal, the build runs with the overlay already sourced, and it can become a build in a polluted environment. The basic way is to separate a "build terminal" and a "run terminal". Do the build in a terminal where nothing (or only the underlay) is sourced, and when the build is done, source the overlay in another terminal and run. Get used to this two-terminal setup.

> Column: You can also stack an overlay on top of another overlay. If you build and source another workspace while one workspace is sourced, the latter looks for its dependency packages in the former. This setup is used when you split a large project into several workspaces, but at first it is less confusing to finish everything in one workspace.

### What you learn: Build and run the examples repository

What you learn: Bring an external ROS 2 package into the workspace and build it.

Preparation: `~/ros2_ws/src` exists, and the underlay is already sourced.

Content:

Before writing a node yourself, let's experience the flow of the build with the official ROS 2 examples repository.

```bash
cd ~/ros2_ws/src
git clone -b jazzy https://github.com/ros2/examples.git
cd ~/ros2_ws
```

You only cloned it into `src`, and you have not built it yet. We use `rosdep` to check that the dependency packages are in place.

```bash
rosdep install --from-paths src --ignore-src -r -y
```

`--from-paths src` means "look at all the `package.xml` files of the packages under `src` and check the dependencies". `--ignore-src` means "ignore the packages in `src` themselves as dependencies" (so that a package is not mistaken as a dependency of itself), and `-y` is an option that automatically answers yes to the confirmation prompt. The examples repository uses only the types and libraries provided by ROS 2 itself, so in many environments nothing new is installed here, but if you go through this step, you can use the same command later when you build an external repository that contains many packages.

Build it.

```bash
colcon build --symlink-install --packages-select examples_rclcpp_minimal_publisher
```

The examples repository contains many packages, but here we narrow the build to the smallest example with `--packages-select`. After the build finishes, source the overlay and run.

```bash
source install/setup.bash
ros2 run examples_rclcpp_minimal_publisher publisher_member_function
```

If lines such as `Publishing: "Hello, world! 0"` keep flowing every second, you succeeded. In another terminal, source in the order underlay → overlay, and check the flowing messages with `ros2 topic list` and `ros2 topic echo /topic`.

**Exercise**: Build the `examples_rclcpp_minimal_subscriber` package with the same steps, run it together with the publisher, and check that the receive log appears in the subscriber's terminal.

<details markdown="1"><summary>Answer</summary>

```bash
colcon build --symlink-install --packages-select examples_rclcpp_minimal_subscriber
source install/setup.bash
ros2 run examples_rclcpp_minimal_subscriber subscriber_member_function
```

If you run this in a terminal different from the one running the publisher, a log such as `I heard: "Hello, world! N"` flows on the subscriber side. It also connects to what you learned in [05_topics](05_topics.md) if you check with `ros2 topic info /topic` that the publisher count and the subscriber count are each 1.

</details>

When you manage many packages with the `vcs` tool, the `version` in a `.repos` file can be either a SHA or a branch name. Some setups write the SHA of a specific submission directly, and do not follow upstream automatically when it moves. On the other hand, if you write a branch name, the contents change depending on the day you run `vcs import`. **When you read a `.repos` file, always check whether it is a SHA or a branch name.** If you pin it with a SHA, you can reduce accidents where it does not reproduce in someone else's environment.

## Going further

By default, `colcon build` tries to build all the packages in the workspace in parallel. When the number of packages grows and your machine gets slow, you can reduce the number of parallel jobs with `--parallel-workers`. Also, `colcon test` runs the tests written in the packages all at once, but how to write tests is a separate topic, so for now just know the name.


## Wrapping up

The structure of a workspace (`src`/`build`/`install`/`log`) and the relationship between underlay and overlay are basic knowledge that you keep using when you develop with ROS 2. When the build feels strange, first suspect these two things: delete `build` and `install` and do a clean build, and check the order of sourcing. If you do not understand something, ask someone experienced or check the official documentation.

Next is [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md), where you write a node with the same structure as the examples you built today.

## References

- [ROS 2 Documentation: Jazzy — Creating a workspace](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Creating-A-Workspace/Creating-A-Workspace.html)
- [ROS 2 Documentation: Jazzy — Using colcon to build packages](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Colcon-Tutorial.html)
- [ros2/examples (jazzy branch)](https://github.com/ros2/examples/tree/jazzy)
- [05_topics](05_topics.md)
- [09_launch_and_ros2_bag](09_launch_and_ros2_bag.md)
- [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md)
