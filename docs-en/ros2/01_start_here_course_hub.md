# ROS 2 Lecture 01: Start Here (Course Hub)

## Introduction

Welcome to the ROS 2 course. This article has no main content. It is a hub that only gives you a map of the whole series and directions. Read it first, and decide where you should start.

The assumed environment is Ubuntu 24.04 / ROS 2 Jazzy. We do not guarantee that things work on other distributions or operating systems.

## Target readers

This series is for readers who have never used ROS 2. It assumes basic C/C++ and basic Linux command-line use (`cd` and `ls` are enough).

The series covers everything from the stage of "I have heard the name ROS 2, but I don't know what it is" to the stage of "building and reading a navigation stack inside my own robot or system".

## Before you start: if you are not confident in C / C++

**This material includes a C++ course to read before ROS 2, and a C course for the microcontroller side.**
If you stopped at "basic C/C++ is assumed" above, do those first.

| Where you are stuck | Where to go |
| --- | --- |
| You freeze when `const` or `static` appears. You mix up pointers and references | [C++ Basics](../cpp-basics/README.md) (10 chapters / exercises `cppb01` to `cppb10`) |
| You cannot read `shared_ptr`, lambdas, `std::move`, or templates | [C++](../cpp/README.md) (15 chapters / exercises `cpp01` to `cpp12`) |

**rclcpp code depends heavily on specific C++ features.** If you start with these unclear,
you cannot tell whether you are learning ROS 2 concepts or fighting C++ syntax.
That is the least efficient state, so we recommend that you finish the C++ side first.

If you are in a hurry, the shortest path is C++ chapters **2 → 3 → 6 → 7 → 11** (about 4 hours).
Just these 5 chapters are enough to enter Part 1 and Part 2 of this ROS 2 course.

The overall picture is in [Overview of the material](../README.md).

## Map of the whole series

There are 24 articles in total: 22 main articles and 2 supplements (`05b` and `15b`). The numbers are the reading order.

**Articles with a `b` are supplements.** You can skip them when you read through the main numbers in order. They help when you actually run into the trouble they cover.

### Part 1: ROS 2 basics (9 articles + 1 supplement)

In this part, you learn the basic ROS 2 concepts, such as nodes, topics, services, parameters, and actions, by typing commands yourself.

1. [01_start_here_course_hub](01_start_here_course_hub.md) (this article)
2. [02_environment_setup](02_environment_setup.md)
3. [03_getting_a_feel_with_turtlesim_and_rqt](03_getting_a_feel_with_turtlesim_and_rqt.md)
4. [04_nodes](04_nodes.md)
5. [05_topics](05_topics.md)
5b. [05b_dds_discovery_and_ros_domain_id](05b_dds_discovery_and_ros_domain_id.md) (read it when several people are on the same network. Countermeasures against crosstalk)
6. [06_services](06_services.md)
7. [07_parameters](07_parameters.md)
8. [08_actions](08_actions.md)
9. [09_launch_and_ros2_bag](09_launch_and_ros2_bag.md)

### Part 2: Package development (8 articles + 1 supplement)

In this part, you learn to write your own ROS 2 packages. It covers both C++ and Python.

10. [10_workspaces_and_colcon](10_workspaces_and_colcon.md)
11. [11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md)
12. [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md)
13. [13_custom_interfaces](13_custom_interfaces.md)
14. [14_implementing_services](14_implementing_services.md)
15. [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md)
15b. [15b_pluginlib](15b_pluginlib.md) (swapping plugins with YAML)
16. [16_implementing_an_action_server](16_implementing_an_action_server.md)
17. [17_testing_and_debugging](17_testing_and_debugging.md)

### Part 3: Intermediate, running a robot (5 articles)

This part collects the knowledge you need to run a real robot: coordinate frames, URDF, ros2_control, sensor integration, and more.

18. [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md)
19. [19_writing_urdf](19_writing_urdf.md)
20. [20_ros2_control_overview](20_ros2_control_overview.md)
21. [21_sensor_integration](21_sensor_integration.md)
22. [22_thinking_about_localization](22_thinking_about_localization.md)

## Recommended learning order

Not everyone needs to read everything. We offer two courses, depending on your goal.

**Minimum course (Part 1 + Part 2, 17 articles)**

This is for people who want to write and run their own nodes in ROS 2. The goal is that you can write one control package and take charge of one function of a real robot (such as a servo control node). Part 1 and Part 2 alone are enough.

**Full course up to a real robot (Part 1 to Part 3, 24 articles)**

This is for people who want to work with real robots, including sensors, coordinate frames, and ros2_control.
Part 3 covers TF2, URDF, ros2_control, sensor integration, and the ideas behind localization.
After this, you have the basic knowledge to move on to developing autonomous mobile robots with ROS 2.

For both courses, the text is written on the assumption that you read in numerical order. If you skip around, you will probably get stuck because you lack the prerequisites. If you are not sure, read in order.

## Using this as a course

This course runs as self-study plus an oral exam.

- Each article has enough information for someone learning this for the first time to read through alone. The articles do not assume that an instructor explains them aloud. "Please read this" basically works.
- Each article has a "Using this as a course" section with oral exam questions (sets of a question and a model answer) and a list of things to prepare for that article's scope. When a learner reports completion, use it to check their understanding.
- Knowledge does not come by waiting. Let learners move their own learning forward, and ask questions only where they are stuck.
- Do not avoid teaching. Teaching others also sharpens your own knowledge. If you are asked something you cannot answer, say so honestly and look it up together.


## References

- [ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/index.html)
