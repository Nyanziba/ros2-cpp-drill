# ROS 2 Lecture 22: How to Think About Localization

## Introduction

After this article, you can explain how to compute "where the robot is on the map now", and which TF passes that result to the current move_base_flex setup.

The prerequisite is that you have finished reading [21_sensor_integration](21_sensor_integration.md). The point we set up as foreshadowing in the previous article, "we need a mechanism that keeps publishing the map→odom transform", is picked up here.

## Lecture goals

- Explain the relation among the map, odom, and base frames, and what localization actually outputs
- State the strengths and weaknesses of wheel odometry, IMU, and LiDAR scan matching
- Explain intuitively "what NDT scan matching optimizes"
- Check the `map → odom → base_link` TF contract that move_base_flex requires

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco set up
- The localization package adopted on the real robot, and an environment where you can check the `map → odom → base_link` TF
- If possible, a recorded rosbag (one that contains odom/imu/scan/map). If you have none, the session will be mostly lecture
- A whiteboard or paper. Keep time to draw the coordinate transform diagram by hand


### Suggested time plan

- Explain the map/odom/base frames and the problem setting: 10 min
- Compare the nature of the three information sources: 15 min
- Intuitive explanation of NDT scan matching: 15 min
- Check the TF connection with move_base_flex: 15 min
- Oral questions: 10 min

### Oral questions

**Q1. What does the localization node compute and publish, in the end?**

Model answer: It estimates the pose in the map frame, and usually outputs `map → odom`. By chaining it with `odom → base_link` from the odometry side, it can resolve the `map → base_link` that move_base_flex needs.

**Q2. Why do we not do localization with wheel odometry alone?**

Model answer: Wheel odometry integrates the movement from the wheel rotation (or the drive command), so small errors such as slip, wheel diameter error, and unevenness of the ground add up over time. This is called drift. It is reliable as a short-time, high-rate relative movement, but it has no reference for the absolute position, so the longer the robot runs, the more the gap from the real position grows.

**Q3. What does NDT scan matching search for? Explain in one sentence.**

Model answer: It represents the point cloud of a pre-built map as a set of normal distributions (Gaussian distributions), one per voxel, and searches for the position and orientation at which the goodness of fit (log-likelihood) is largest when the scan just acquired is overlaid on those distributions.

**Q4. What do you make the common interface, so that localization can be swapped without touching move_base_flex?**

Model answer: Not the internal state or the estimation algorithm of each implementation, but the `map → odom → base_link` TF and the required time consistency are made the common contract.

## Main content

### The three frames: map, odom, and base

Before we talk about localization, let us sort out the frames the robot has. In the ROS 2 navigation stack, the following three TF frames are the basis.

- `map`: a frame fixed to the world that does not move. The origin of the pre-built map
- `odom`: the origin of odometry. It takes the robot position at startup as the origin, and gives the relative position from there by integrating the wheels and IMU
- `base_link`: a frame fixed to the robot body

`odom → base_link` is computed by wheel odometry (and the IMU in some cases). The problem is `map → odom`. Nobody can measure it directly. Seen from the origin of the map, where am I now? Keeping on finding this is the job of localization.

The current move_base_flex setup also assumes that this TF tree holds. Do not look at the internal implementation of the localization; check that `tf2_echo map base_link` can be resolved, including the time.

### The three information sources and their nature

There are mainly three information sources you can use for localization. Each has different strengths and weaknesses, so do not rely on one; combine them.

| Source | Rate | Feature | Weakness |
|---|---|---|---|
| Wheel odometry | High rate (tens to 100 Hz) | Gives short-time relative movement with high accuracy | Drifts because slip and errors are integrated. Has no reference for absolute position |
| IMU | High rate (100 Hz or more) | Angular velocity is relatively reliable | The angle also drifts when integrated. Position estimation from acceleration diverges even faster |
| LiDAR scan matching | Low rate (a few to over ten Hz, depending on the computation load) | Gives the absolute position. Does not drift | Heavy computation. In environments with few features (such as only long straight walls), even a wrong pose can match |

Wheel odometry and IMU are high rate, but integration errors accumulate. LiDAR scan matching, on the other hand, compares the pre-built map directly with the current scan, so it can be used for global correction. But in general it needs a lot of computation and its update rate is low.

Typically, you predict with the high-rate odometry and IMU, and correct with the low-rate LiDAR matching result. For the concrete implementation, the localization package adopted on the real robot is the authority.

### Intuition for NDT scan matching

A representative method of LiDAR scan matching is NDT (Normal Distributions Transform). Here is an intuitive explanation of how it works.

1. Divide the point cloud of the pre-built map into voxels (a grid of cubes) of a fixed size
2. Approximate the distribution of the points in each voxel by one normal distribution (Gaussian distribution). In other words, the whole map is replaced by "a set of normal distributions"
3. Transform each point of the LiDAR scan just acquired into the map frame, using a candidate position and orientation
4. For each transformed point, sum over all points how "likely" it is under the normal distribution of the voxel it falls in (log-likelihood)
5. Search for the position and orientation that maximize this sum, with a gradient method (such as Newton's method)

In short, it looks for "the position and orientation at which the current scan overlays best on the shape that the pre-built map expects", by numerical optimization. The role is similar to ICP (Iterative Closest Point), which compares raw point clouds by distance. But NDT compares the distribution per voxel, not the points themselves, so it tends to be robust to variation in point cloud density and to converge faster.

> Column: NDT compresses the point cloud into distributions, so you do not need to carry the whole map point cloud around every time. The fact that even a large map can be handled relatively lightly is thanks to this pre-computation.

### Connecting to the navigation stack

In a typical navigation stack, the localization implementation is separate from the path planning and plugin layers. The localizer itself is not included in move_base_flex. The contract that move_base_flex needs is not the kind of localization method, but that the following TF keeps being resolvable.

```text
map → odom → base_link
```

Choose a localization node separately according to the sensor setup, and supply its final result as TF. move_base_flex gets the current pose with `global_frame` (default `map`) and `robot_frame` (default `base_link`). Note that it does not assume its own `/pose` topic.

Use the following for checking.

```bash
ros2 run tf2_ros tf2_echo map base_link
ros2 topic hz /odom
ros2 topic hz /imu
```

The explanation of the principles of NDT and EKF stays usable even if the localization implementation you adopt changes. But for concrete topic names, QoS, and how to set the initial pose, check the README and launch files of the localization package chosen for the real robot, which are the authority.

### Exercises

**Exercise 1**: Start the localization you use on the real robot, and check that `ros2 run tf2_ros tf2_echo map base_link` keeps updating.

**Exercise 2**: When you stop the localization, check with which outcome a goal sent to `/move_base_flex/move_base` fails, and match it with the TF error logs.

**Exercise 3**: Check `global_frame` and `robot_frame` in the navigation configuration files, and draw a diagram showing whether they match the TF tree of the real robot.

## Going further

- **AMCL**: A particle-filter localization that comes as standard in ROS 2 Navigation2. It scatters many pose hypotheses (particles) over the occupancy grid map, and narrows down the position by repeating weighting and resampling according to how well they match the sensor observations. It searches for the solution by probabilistic sampling, not by a gradient method like NDT, so it converges easily even when the initial position is far off.
- **robot_localization (EKF/UKF)**: A general-purpose package that can fuse several odometry and IMU sources in any combination. A typical navigation stack separates the localization implementation from move_base_flex, so it is a strong choice.
- **The difference from SLAM**: Localization is the problem of "finding your position using an existing map", and SLAM (Simultaneous Localization and Mapping) is the problem of "building the map while also finding your position at the same time". Distinguish the stage of building a map (exploration runs) from the stage of running with the finished map (the competition itself and so on) as separate problems. If you mix them up, you fall into off-target questions such as "why is the map not being updated now?".


## Conclusion

Localization comes down to one phrase, "keep publishing map→odom", but behind it are accumulated decisions based on understanding the nature of several sensors and combining them. This time we looked at the ideas of EKF and NDT, and the TF contract that move_base_flex requires. In the next article, we will see how this localization is used in the whole of autonomous navigation. If you do not understand something, ask a team member, or refer to the documentation of the related packages.

Next is "23_overview_of_autonomous_navigation" (an advanced edition, not included in this material), where Part 4 begins.

## References

- [ROS 2 Documentation: Jazzy — tf2](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Tf2/Tf2-Main.html)
- [21_sensor_integration](21_sensor_integration.md)
- "23_overview_of_autonomous_navigation" (an advanced edition, not included in this material)
- The official documentation of move_base_flex (TF requirements)
