# ROS 2 Lecture 17: Testing and Debugging

## Introduction

After this article, you will be able to find out by yourself "where it is stuck" when a node does not work. You will also be able to write logic that does not depend on ROS as ordinary unit tests, and run them all together with colcon.

Part 2 (nodes to actions, and launch) ends here. How to check that what you implemented works correctly is the theme this time.

## Goals

- Use the log levels (DEBUG/INFO/WARN/ERROR) correctly to follow the state of a node
- Trace a problem from upstream and isolate it, with `rqt_console`, `ros2 doctor`, and `ros2 topic echo/hz`
- Write minimal tests with pytest (ament_python) and gtest (ament_cmake), and run them with `colcon test`
- Understand the design policy of moving logic that does not depend on ROS out into ordinary functions

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy already set up ([02_environment_setup](02_environment_setup.md) finished)
- `rqt_console` (if it is not installed, run `sudo apt install ros-jazzy-rqt-console`)
- `ros2 doctor` is bundled with ROS 2 itself, so no extra install is needed
- An environment where pytest works (building an ament_python package includes pytest hooks, so it is usually installed automatically through colcon. To run it alone by hand, use `pip install pytest`)
- Broken sample nodes (it is effective to prepare and show cases beforehand, such as only one of two nodes not running, or a typo in a topic name)

### Suggested time plan

- Explain log levels and RCLCPP_INFO/get_logger(): 10 min
- Isolation exercise with rqt_console/ros2 doctor/echo/hz: 20 min
- Write and run a minimal pytest example: 15 min
- Write a minimal gtest example and run `colcon test`: 15 min
- Talk about ROS-independent design, introduce CI, and oral questions: 10 min

### Oral questions

**Q1. What is the difference between `RCLCPP_INFO` and `RCLCPP_DEBUG`? Even if you put DEBUG logs in the code, they sometimes do not appear in production. Why?**

Model answer: Both differ only in log level, and importance goes up in the order `RCLCPP_DEBUG` < `RCLCPP_INFO` < `RCLCPP_WARN` < `RCLCPP_ERROR` < `RCLCPP_FATAL`. The default log level of a node is often INFO or above, so DEBUG logs are not printed by default. To show them, you need to lower the log level explicitly, such as `ros2 run <package> <node> --ros-args --log-level debug`. If you leave DEBUG logs on all the time in production, the output grows large and the important WARN/ERROR lines get buried, so the basic rule is to keep INFO or above in normal use.

**Q2. When a topic is not connected, what does "trace from upstream" mean as a procedure? Explain it concretely.**

Model answer: First check with `ros2 node list` that the target node is up. If it is not up, the node itself failed to start, so look at the standard output and logs of that node. If the node is up, check with `ros2 topic list` whether the topic exists. If it does not exist, check whether the publisher code has a typo in the topic name. If the topic exists, check the number of publishers/subscribers and the QoS with `ros2 topic info -v <topic>`, and see with `ros2 topic hz <topic>` whether data really flows. If data flows, suspect the content of the subscriber callback. In this order you check "from the side closer to the source of the data". On the other hand, if you start by suspecting the receiving code, you notice late a simple mistake such as the publisher not running at all.

**Q3. What does `ros2 doctor` check?**

Model answer: It is a tool that diagnoses the health of the ROS 2 environment itself in one go (version consistency of the installed packages, network settings, the setting of `ROS_DOMAIN_ID`, warnings about running topics, and so on). It cannot find bugs in the logic of a particular node, but you use it to rule out early the possibility that "the environment itself is wrong". `ros2 doctor --report` gives a detailed report.

**Q4. Why do we move logic that does not depend on ROS out into ordinary functions?**

Model answer: A test that goes through an `rclpy` or `rclcpp` node needs the node to start and run, so it is heavy to write and to run. If you move pure computation (finding the curvature of a path, recovering the yaw angle from an orientation, parsing a waypoints YAML, and so on) out into ordinary functions, you can check it quickly with plain pytest or gtest without starting ROS at all. As a side benefit, it also becomes easier to reuse the logic and to port it to other projects.

## Main text

### What you learn: Using log levels correctly

What you learn: write with log levels in mind, using the `RCLCPP_INFO` family of macros (C++) and `self.get_logger()` (Python).

Preparation: prepare one node in whichever of C++ or Python you usually use.

Details:

ROS 2 logs have five levels. From the lowest importance, the order is `DEBUG < INFO < WARN < ERROR < FATAL`. In C++ you write it like this.

```cpp
RCLCPP_DEBUG(this->get_logger(), "cmd_vel computed: v=%.2f w=%.2f", v, w);
RCLCPP_INFO(this->get_logger(), "goal reached");
RCLCPP_WARN(this->get_logger(), "path is empty, skipping this cycle");
RCLCPP_ERROR(this->get_logger(), "failed to load costmap: %s", e.what());
```

In Python, you call the methods of the same names from `self.get_logger()`.

```python
self.get_logger().debug(f"cmd_vel computed: v={v:.2f} w={w:.2f}")
self.get_logger().info("goal reached")
self.get_logger().warn("path is empty, skipping this cycle")
self.get_logger().error(f"failed to load costmap: {e}")
```

The basic use is: DEBUG for detailed state checks during development, INFO for "milestones that show it runs normally", WARN for "not abnormal, but a state you should care about", and ERROR for "a failure where the work cannot continue". If you fill everything with INFO, the logs that really matter get buried and you cannot find them.

Hint: you can change the log level at node start-up with `--ros-args --log-level debug`. Use it like this: INFO or above in normal use, and look into DEBUG only while debugging.

### What you learn: Listing logs with rqt_console

What you learn: see the logs of several nodes together on one screen.

Preparation: start a few nodes. turtlesim + teleop is fine too.

Details:

It is hard to line up many terminals and follow the logs by eye. If you start `rqt_console`, the logs of the running nodes are gathered in one GUI window, and you can filter by level and search.

```bash
rqt_console
```

For problems that involve several nodes (for example, when you want to follow "when this node's WARN appears" together with the movement of other nodes), this is faster than following terminals one by one.

### What you learn: Suspecting the environment with ros2 doctor

What you learn: know the procedure of suspecting the environment itself before the node's logic.

Preparation: nothing in particular.

Details:

When "it worked until yesterday but not today" or "it works on another PC but not on mine", it is sometimes faster to suspect a difference in the environment than a bug in the code.

```bash
ros2 doctor
```

`ros2 doctor --report` gives more detailed information (OS, ROS distribution, network settings, and so on). If you first rule out problems that "you cannot notice even by reading the code", such as `ROS_DOMAIN_ID` unintentionally overlapping with another environment, or versions of required packages that do not match, the later debugging does not take longer than necessary.

### What you learn: Tracing from upstream to isolate the problem

What you learn: learn the isolation procedure that follows "up to which node the data arrives" from the source.

Preparation: it is good practice to prepare an intentionally broken node setup (one node not started, a typo in a topic name, and so on).

Details:

When a topic does not connect or a value is wrong, tracing the flow of data from upstream is more reliable than reading code at random. The procedure is as follows.

1. `ros2 node list` — is the target node up?
2. `ros2 topic list` — does the target topic exist?
3. `ros2 topic info -v <topic>` — do the number of publishers/subscribers and the QoS match?
4. `ros2 topic hz <topic>` — does data really flow?
5. `ros2 topic echo <topic>` — is the flowing value itself correct?

This order is important. If you start by suspecting the receiving callback, you tend to miss a simple cause such as the publisher not running at all. If you check from the side closer to the source of the data, you quickly find the boundary of "up to here it is normal, and from here it is broken".

**Practice problem**: If you run `ros2 topic echo /turtle1/cmd_vel` without starting teleop, nothing is shown. From this state, run the five steps above in order, and explain where the boundary of what you can judge as "normal" is.

<details markdown="1"><summary>Answer</summary>

`ros2 node list` shows turtlesim_node (normal). `ros2 topic list` shows `/turtle1/cmd_vel` (normal; the topic itself is defined). `ros2 topic info -v /turtle1/cmd_vel` shows Publisher count: 0. This is the boundary: you learn that the topic exists but there is no publisher at all. It is a natural result that neither `hz` nor `echo` shows anything, and you do not need to investigate with these steps in the first place. The cause is only that "teleop is not started", and it is not a bug in the code.

</details>

### What you learn: Writing minimal unit tests with pytest (ament_python)

What you learn: add pytest-based unit tests to an ament_python package.

Preparation: prepare the layout of an ament_python package (`setup.py`, `<package>/`, and a `test/` directory).

Details:

In an ament_python package, if you put `test_*.py` in the `test/` directory, `colcon test` runs pytest automatically. This is a test example for functions that do not depend on ROS.

```python
# test/test_yaw_utils.py
import math
import pytest

from my_package.yaw_utils import yaw_to_quaternion, yaw_from_quaternion


def test_yaw_to_quaternion_zero():
    assert yaw_to_quaternion(0.0) == pytest.approx((0.0, 0.0, 0.0, 1.0))


def test_yaw_round_trip():
    for yaw in [0.0, 0.5, -0.5, math.pi - 0.01]:
        qx, qy, qz, qw = yaw_to_quaternion(yaw)
        assert yaw_from_quaternion(qx, qy, qz, qw) == pytest.approx(yaw, abs=1e-9)
```

`yaw_to_quaternion` and `yaw_from_quaternion` are ordinary functions that do not import `rclpy` at all. You can run just this test alone without starting a node.

```bash
pytest test/test_yaw_utils.py -v
```

### What you learn: Writing minimal unit tests with gtest (ament_cmake)

What you learn: add GoogleTest-based unit tests to an ament_cmake package.

Preparation: prepare the layout of an ament_cmake package (`CMakeLists.txt`, `src/`, `test/`).

Details:

On the C++ side we use GoogleTest (gtest). Add settings like the following to `CMakeLists.txt`.

```cmake
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_path_utils test/test_path_utils.cpp)
  target_link_libraries(test_path_utils path_utils)
endif()
```

The test itself looks like this.

```cpp
// test/test_path_utils.cpp
#include <gtest/gtest.h>
#include "my_package/path_utils.hpp"

TEST(PathUtilsTest, CurvatureOfStraightLineIsZero)
{
  std::vector<Point2D> straight_path = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}};
  EXPECT_NEAR(computeMaxCurvature(straight_path), 0.0, 1e-9);
}

TEST(PathUtilsTest, EmptyPathReturnsZero)
{
  std::vector<Point2D> empty_path;
  EXPECT_EQ(computeMaxCurvature(empty_path), 0.0);
}
```

`computeMaxCurvature` is moved out into `path_utils.hpp` as an ordinary function that does not inherit from `rclcpp::Node`. You do not need to start a node, so once the build is done, you get the result in milliseconds.

### What you learn: Running everything together with colcon test

What you learn: run all the tests in the workspace at once and check the results.

Preparation: the package that contains the pytest and gtest tests must be built.

Details:

After you build the packages individually, run the tests together with `colcon test`.

```bash
colcon build
colcon test
```

You can check the details of the results (what is inside the failed tests) with `colcon test-result`.

```bash
colcon test-result --verbose
```

Without `--verbose` you only get the fact that "it failed" and you cannot tell which assertion failed, so when a test fails, always run it again with `--verbose`. If you want to test only a specific package, you can narrow it down with `colcon test --packages-select <package_name>`.

**Practice problem**: You wrote `colcon test` in the CI script, but CI turned green even though a test failed. Why?

<details markdown="1"><summary>Full program that produced this output</summary>

```xml
<?xml version="1.0"?>
<!-- ~/ros2_ws/src/failpkg/package.xml -->
<?xml-model href="http://download.ros.org/schema/package_format3.xsd" schematypens="http://www.w3.org/2001/XMLSchema"?>
<package format="3">
  <name>failpkg</name>
  <version>0.0.0</version>
  <description>Package with one deliberately failing gtest</description>
  <maintainer email="you@example.com">your_name</maintainer>
  <license>Apache-2.0</license>

  <buildtool_depend>ament_cmake</buildtool_depend>

  <test_depend>ament_cmake_gtest</test_depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
```

```cmake
# ~/ros2_ws/src/failpkg/CMakeLists.txt
cmake_minimum_required(VERSION 3.8)
project(failpkg)

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(t test/t.cpp)
endif()

ament_package()
```

```cpp
// ~/ros2_ws/src/failpkg/test/t.cpp
#include <gtest/gtest.h>
TEST(A, DeliberatelyFails) {
  EXPECT_EQ(1, 2) << "This is an intentional failure";
}
TEST(A, Passes) {
  EXPECT_EQ(1, 1);
}
```

```bash
cd ~/ros2_ws
colcon build
colcon test
echo $?
colcon test-result --verbose
```

</details>

<details markdown="1"><summary>Answer</summary>

Because `colcon test` **returns exit code 0 even when tests fail**. CI judges pass or fail by the exit code, so it cannot detect the failure this way.

If you actually check with a package that contains one gtest that fails on purpose, you get this.

<!-- measure: env=ros files=src/failpkg/package.xml,src/failpkg/CMakeLists.txt,src/failpkg/test/t.cpp cmd="colcon build >/dev/null 2>&1; colcon test; exit_code=$?; echo '$ echo $?'; echo $exit_code" -->
```
Starting >>> failpkg
--- stderr: failpkg
Errors while running CTest
Output from these tests are in: /home/ubuntu/ros2_ws/build/failpkg/Testing/Temporary/LastTest.log
Use "--rerun-failed --output-on-failure" to re-run the failed cases verbosely.
---
Finished <<< failpkg [0.45s]	[ with test failures ]

Summary: 1 package finished [0.65s]
  1 package had stderr output: failpkg
  1 package had test failures: failpkg
$ echo $?
0
```

The screen shows `[ with test failures ]` and `1 package had test failures`, so a human can notice it when reading. But the exit code is 0. For `colcon test`, "the job of running the tests itself was completed", so it counts as success.

There are two fixes.

```bash
# (a) Give colcon test an explicit flag (the exit code becomes 1)
colcon test --return-code-on-test-failure

# (b) Separate running from judging. test-result returns exit code 1 if there is a failure
colcon test
colcon test-result --verbose
```

In CI, always include either (a) or (b). A CI that only has `colcon test` is the same as verifying nothing.

Also, if you add `--verbose` to `colcon test-result`, it shows even which assertion failed. Without it, you only get the fact "1 failure".

<!-- measure: env=ros files=src/failpkg/package.xml,src/failpkg/CMakeLists.txt,src/failpkg/test/t.cpp cmd="colcon build >/dev/null 2>&1; colcon test >/dev/null 2>&1; colcon test-result --verbose" -->
```
build/failpkg/Testing/20261002-1117/Test.xml: 1 test, 0 errors, 1 failure, 0 skipped
- t
  <<< failure message
    -- run_test.py: invoking following command in '/home/ubuntu/ros2_ws/build/failpkg':
     - /home/ubuntu/ros2_ws/build/failpkg/t --gtest_output=xml:/home/ubuntu/ros2_ws/build/failpkg/test_results/failpkg/t.gtest.xml
    Running main() from /opt/ros/jazzy/src/gtest_vendor/src/gtest_main.cc
    [==========] Running 2 tests from 1 test suite.
    [----------] Global test environment set-up.
    [----------] 2 tests from A
    [ RUN      ] A.DeliberatelyFails
    /home/ubuntu/ros2_ws/src/failpkg/test/t.cpp:4: Failure
    Expected equality of these values:
      1
      2
    This is an intentional failure
    
    [  FAILED  ] A.DeliberatelyFails (2 ms)
    [ RUN      ] A.Passes
    [       OK ] A.Passes (0 ms)
    [----------] 2 tests from A (2 ms total)
    
    [----------] Global test environment tear-down
    [==========] 2 tests from 1 test suite ran. (6 ms total)
    [  PASSED  ] 1 test.
    [  FAILED  ] 1 test, listed below:
    [  FAILED  ] A.DeliberatelyFails
    
     1 FAILED TEST
    -- run_test.py: return code 1
    -- run_test.py: inject classname prefix into gtest result file '/home/ubuntu/ros2_ws/build/failpkg/test_results/failpkg/t.gtest.xml'
    -- run_test.py: verify result file '/home/ubuntu/ros2_ws/build/failpkg/test_results/failpkg/t.gtest.xml'
  >>>
build/failpkg/test_results/failpkg/t.gtest.xml: 2 tests, 0 errors, 1 failure, 0 skipped
- failpkg.A DeliberatelyFails (/home/ubuntu/ros2_ws/src/failpkg/test/t.cpp:3)
  <<< failure message
    /home/ubuntu/ros2_ws/src/failpkg/test/t.cpp:4
    Expected equality of these values:
      1
      2
    This is an intentional failure
  >>>

Summary: 3 tests, 0 errors, 2 failures, 0 skipped
```

Both `Test.xml` (ctest's own result) and `t.gtest.xml` (gtest's result) are counted, so the Summary says 3 tests / 2 failures: the same failure is counted twice.

</details>

### What you learn: A design that separates logic that does not depend on ROS

What you learn: understand the design policy that makes code easy to test.

Preparation: nothing in particular.

Details:

The pytest and gtest examples so far have one thing in common. The functions under test do not inherit from `rclpy.node.Node` or `rclcpp::Node`; they are just functions. If you try to test a ROS node, you need `rclpy.init()`, starting the node, and spinning the executor, and writing even one test suddenly becomes heavy.

So a good design is to move the logic computed in node callbacks (curvature calculation of a path, coordinate transformation, YAML parsing, judging state transitions, and so on) out of the node class as far as possible, and write it as ordinary functions or classes. The node side becomes a thin layer that only calls those functions and does publish/subscribe. Then you can test the logic without starting ROS, and on the node side you only need to check "is it connected properly".

This idea is one reason why many navigation libraries are built as C++/Python libraries that do not depend on ROS at all. Because the layer that wraps thinly as a move_base_flex plugin is separated from the layer that does path planning and advanced algorithms, you can write unit tests that do not start ROS at all as they are. We will cover this kind of separation in detail in the advanced section of Part 4.

> Column: The tests of advanced navigation libraries really have a three-layer structure: "pure unit tests → state-transition scenario tests → simulator E2E". The idea is to use ROS-independent functions as the base, and stack tests with a higher level of integration on top of it.

### What you learn: An example from a real project

What you learn: see how tests and CI are set up in a real robotics project.

Preparation: nothing in particular.

Details:

In a full-scale navigation project, gtests such as `test_core_smoke.cpp` and `test_planning.cpp` are placed in `tests/`, and pytests such as `test_smoke.py` and `test_bindings.py` are placed separately in `tests_python/`. The procedure to run them locally is summarized in `docs/contributing/testing.md`. On the C++ side it is

```bash
cmake -S . -B build -DNAV_CORE_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

On the Python side it is

```bash
pip install '.[sim,test]'
MPLBACKEND=Agg pytest
```

(from `docs/contributing/testing.md`). The same file also states the three-layer structure of the test strategy: "pure unit tests (GoogleTest/pytest) → state-management scenario tests → simulator E2E".

On the plugin layer side, pytests are placed in `tools/test/test_utils.py`, `sim/test/test_kinematics.py`, and so on. CI is defined as GitHub Actions in `.github/workflows/ci.yml`. In the workflow of the core library, it builds combinations of several platforms (Ubuntu/macOS) and feature flags (MPPI ON/OFF and so on) as a matrix, and runs the C++ tests with `ctest --test-dir build --output-on-failure` and the Python tests with `pytest`. In the workflow of the plugin layer, there is a job that runs pytest per package, such as `pytest tools/test -v`, and a separate `sim-e2e` job that actually starts ROS 2 nodes and checks whether the robot can reach the goal. If tests run automatically in CI every time someone pushes, you can notice broken code before the review.

## Going further

This time we kept to `colcon test` alone, but real CI also includes build caches (using `actions/cache@v4` to cache ccache and prebuilt dependency packages) and even E2E tests with a simulator. How to write GitHub Actions itself, and the idea of building YAML validation into CI (for example, a job that validates the schema of config files), would each take a whole CI/CD lecture if we dug into them. Here we only introduce "how it is used in real robotics projects", and leave how to build CI itself to another article.


## Conclusion

For debugging, just checking "how far is it normal" from upstream often gives you an idea of the cause. Writing tests may look troublesome, but if you move ROS-independent logic out, it does not actually take much effort. This is the end of Part 2.

In the next Part 3, starting from [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md), we move on to how to handle coordinate transformation of robots.

### Matching exercise

After reading this chapter, practice with the matching drill.

- `11_node_test` — Write tests

```bash
./drill run 11
```

From an exercise, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 Jazzy Documentation — Testing](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Testing/Testing-Main.html)
- [ROS 2 Jazzy Documentation — Logging](https://docs.ros.org/en/jazzy/Concepts/Basic/About-Logging.html)
- [16_implementing_an_action_server](16_implementing_an_action_server.md)
- [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md)
