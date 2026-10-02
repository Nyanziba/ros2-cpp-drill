# ROS 2 Lecture 21: Sensor Integration

## Introduction

After this article, you can receive the data of real sensors such as LiDAR and IMU as standard ROS 2 messages and visualize it in RViz2.

The prerequisites are [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md) (the relation between frame_id and tf) and [20_ros2_control_overview](20_ros2_control_overview.md). In sensor integration, people get stuck on how coordinate frames are handled rather than on the message contents, so if your understanding of TF is shallow, you will stumble in the second half of this article.

## Lecture goals

- Understand the structure of the main `sensor_msgs` types (LaserScan, PointCloud2, Imu, JointState) and the role of frame_id
- Explain what topics and message formats a LiDAR driver (using the Livox family as the example) publishes its data in
- When sensor data does not show up in RViz2, suspect a mismatch between Fixed Frame and frame_id, and fix it
- Understand why a conversion node such as `pointcloud_to_laserscan` exists

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco set up
- A bag file, or a real LiDAR/IMU (whichever is available. If you have no real device, replace it with bag playback)
- `ros-jazzy-rviz2` and `ros-jazzy-pointcloud-to-laserscan` (`sudo apt install ros-jazzy-pointcloud-to-laserscan`)
- If you use the Livox driver, build `livox_ros_driver2` beforehand (avoid building on the day, because it takes time)


### Suggested time plan

- Explain the sensor_msgs types and frame_id/stamp: 15 min
- Look at real LiDAR/IMU drivers: 15 min
- Demo of visualization in RViz2 and common pitfalls: 15 min
- Task (show a point cloud from a bag or a driver): 15 min
- Oral questions: 10 min

### Oral questions

**Q1. What happens if you set `header.frame_id` of `sensor_msgs/msg/PointCloud2` to a wrong frame name?**

Model answer: In RViz2, if no frame with that name exists in the tf tree, you get an error "cannot transform from the Fixed Frame" and the point cloud is not shown. If the frame exists but is different from the real sensor position, the transform itself works, but the position of the point cloud seen from the robot frame is shifted. If tf is not connected, you are simply stuck. And even when tf is connected, a wrong frame breaks the input of localization in a way that is hard to notice.

**Q2. Explain why Best Effort QoS tends to be used for LiDAR topics, and what happens when this is not reflected on the RViz2 side.**

Model answer: For data that keeps flowing at a high rate, even if you drop one message the next one comes, so low latency is preferred over paying the cost of retransmission (see [05_topics](05_topics.md)). If the RViz2 subscriber stays Reliable, it does not match the publisher's Best Effort QoS. The topic itself is visible in `ros2 topic list`, but nothing appears in the display panel. You need to set the QoS to Best Effort in the Display settings of RViz2.

**Q3. Why is a node such as `pointcloud_to_laserscan` needed? Can't we just use the PointCloud2 of a 3D LiDAR as it is?**

Model answer: Many 2D obstacle avoidance and SLAM algorithms are built on the assumption of `sensor_msgs/LaserScan` (an array of distances on one plane). If you pass the PointCloud2 of a 3D LiDAR as it is, the computation grows, and some algorithms do not support it. `pointcloud_to_laserscan` slices the point cloud in a specified height range and converts it to the LaserScan format, so that it can be put on an existing 2D-based navigation stack.

## Main content

### Topic: Know the standard sensor_msgs messages

Topic: Understand how ROS 2 expresses sensor data as types.

Preparation: None.

Content:

ROS 2 has standardized message types for each kind of sensor, collected in the `sensor_msgs` package. You can also send sensor data with your own message types. But if you use the standard types, existing tools such as RViz2, nav2, and SLAM tools connect as they are, so you should use the standard types unless you have a special reason.

Let us look at four representative ones.

```bash
ros2 interface show sensor_msgs/msg/LaserScan
```

```
std_msgs/Header header
float32 angle_min
float32 angle_max
float32 angle_increment
float32 time_increment
float32 scan_time
float32 range_min
float32 range_max
float32[] ranges
float32[] intensities
```

`LaserScan` is the type for a 2D LiDAR that scans one plane. The distances measured from `angle_min` to `angle_max` in steps of `angle_increment` are lined up in the `ranges` array. Note that the array index only corresponds to an angle, and there are no x, y values as coordinates.

```bash
ros2 interface show sensor_msgs/msg/PointCloud2
```

`PointCloud2` is the point cloud that a 3D LiDAR or a depth camera outputs. It is not a simple array like `ranges`. It is a variable format that packs (x, y, z, intensity, ...) into a binary buffer. The field layout is described in `fields`, so the basic way is to read it through utilities of `sensor_msgs_py` or pcl rather than reading it directly.

```bash
ros2 interface show sensor_msgs/msg/Imu
```

```
std_msgs/Header header
geometry_msgs/Quaternion orientation
float64[9] orientation_covariance
geometry_msgs/Vector3 angular_velocity
float64[9] angular_velocity_covariance
geometry_msgs/Vector3 linear_acceleration
float64[9] linear_acceleration_covariance
```

`Imu` has the orientation (quaternion), angular velocity, linear acceleration, and the covariance matrix of each. Many IMU drivers fill all the covariances with 0, but for sensor fusion (EKF and so on) this is exactly the weight of "how much to trust this sensor". If it stays filled with 0, the fusion side cannot weight it correctly. So even for a driver that cannot get real values, you should put at least the diagonal entries with a rough "about this much" value.

```bash
ros2 interface show sensor_msgs/msg/JointState
```

`JointState` is a type that holds the joint angle, angular velocity, and torque of encoders and servos, with names. It is information from the actuator side rather than a sensor. But it is used as the input with which `robot_state_publisher` updates TF, so we cover it together in this article.

### Topic: Why frame_id and stamp matter

Topic: Understand what `frame_id` and `stamp` of `std_msgs/Header` are for.

Preparation: You have read [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md).

Content:

All four messages above have `std_msgs/Header header`.

```
builtin_interfaces/Time stamp
string frame_id
```

`frame_id` is a string that says "which coordinate frame this data was measured in". Put a frame name that really exists in the tf tree, such as `lidar_link` for a LiDAR point cloud, or `base_link` for values relative to the robot body. RViz2 and other nodes look at this `frame_id` and search the tf tree for "how to transform between it and the Fixed Frame I want to display now". If the frame_id is wrong, or that frame does not exist in the tf tree, the transform cannot be done and the data is simply discarded.

`stamp` is the time at which the data was really acquired. When you combine data from several sensors (for example, integrating LiDAR and IMU with the orientation at the same time), if this `stamp` is off, you cannot tell which moment's values should be matched. Some sensor driver implementations take care to put into `stamp` not the time when the data was received, but "the moment when the sensor really measured". A node where this is off can cause a mysterious accuracy drop in the SLAM or localization that comes after it, so remember this.

> Column: `frame_id` and `stamp` look like magic spells at first glance, but in practice most bugs in sensor integration come from these two. When you feel "the topic is flowing but it does not show in RViz2" or "accuracy drops when I integrate several sensors", suspect these two first.

### Topic: General use of LiDAR drivers

Topic: Understand how the ROS 2 driver of a commercial LiDAR outputs data, using the Livox MID360 as the example.

Preparation: None.

Content:

When you use a 3D LiDAR, many makers provide their own ROS 2 driver package. For Livox, a package called `livox_ros_driver2` is officially distributed. It talks to the sensor over serial/Ethernet and pushes the data into ROS 2 topics.

In addition to the standard `sensor_msgs/PointCloud2`, `livox_ros_driver2` can also output point clouds in its own type, `livox_ros_driver2/msg/CustomMsg`. `CustomMsg` is a format in which each point carries a timestamp, a tag, and a line number. It keeps finer information than the standard `PointCloud2`, but it does not connect as it is to standard tools (RViz2, nav2, and so on). If you set the output to the standard `PointCloud2`, it rides on the existing toolchain as it is, so unless you have a special reason, using the `PointCloud2` output is safe.

```bash
ros2 topic list | grep -i livox
```

Typically it flows on topic names such as `/livox/lidar` (PointCloud2) and `/livox/imu` (for models with a built-in IMU). The actual topic names and QoS settings can be changed in the driver's config files (the yaml files under the `config` directory), so before you use it, make a habit of checking the QoS and the type once with `ros2 topic info -v <topic name>`.


### Topic: IMU frame orientation and covariance

Topic: Understand the frame orientation and the handling of covariance that you should check when you use IMU data.

Preparation: None.

Content:

An IMU is a sensor whose axis definition often differs depending on how it is mounted. The ROS 2 convention expects x forward, y left, z up (right-handed). But if you stick an IMU board on the robot as it is, the axes are often reversed or rotated by 90 degrees. Some drivers correct the axes before they output, but if not, you need to add a `static_transform_publisher` to tf, which expresses the static rotation between the IMU frame and `base_link`, and absorb the frame mismatch on the tf side.

```bash
ros2 run tf2_ros static_transform_publisher --x 0 --y 0 --z 0 --roll 0 --pitch 0 --yaw 1.5708 --frame-id base_link --child-frame-id imu_link
```

This command is an example of the correction when the IMU is mounted rotated by 90 degrees around the z axis. The real values change with how the sensor is mounted, so first check the mounting angle on CAD or on the real robot, and then decide.

As mentioned in the previous section, the covariance is exactly the weight that you pass to a sensor fusion node such as an EKF. If you look at the settings of a localization node such as `ekf_params`, many of them specify the reliability of each sensor with diagonal entries, like `process_noise_diag`. If the covariance on the IMU side is not filled in correctly, this weighting does not work.

### Topic: Visualize with RViz2

Topic: Show sensor data in RViz2 and clear the typical pitfalls.

Preparation: Get ready so that you can play the LiDAR driver or a bag.

Content:

Start RViz2.

```bash
rviz2
```

From "Add" at the top left, add a `LaserScan` or `PointCloud2` Display, and set Topic to the real topic name (for example `/scan` or `/points`). The most common pitfall here is **a wrong Fixed Frame setting**.

If `Fixed Frame` under "Global Options" in the left panel of RViz2 is a frame name that is not connected by tf to the `frame_id` of the sensor message, a red error mark appears in the Display panel and nothing is shown. The three common mistakes are these.

- `Fixed Frame` is `map`, but SLAM/AMCL is not running yet, so the `map` frame does not exist in the tf tree
- The sensor `frame_id` is `lidar_link`, but `lidar_link` is not registered as a transform seen from `base_link` by `static_transform_publisher` or `robot_state_publisher`
- For a topic with Best Effort QoS, the QoS on the RViz2 Display side stays Reliable and does not match (see the QoS part of [05_topics](05_topics.md))

The first step of checking is to look at the tf tree with `ros2 run tf2_tools view_frames` or `ros2 run rqt_tf_tree rqt_tf_tree`. Check here whether the sensor frame is isolated, and whether it is connected up to the name you set as `Fixed Frame`.

**Exercise**: Suppose a red error appears on the point cloud Display while `Fixed Frame` is `odom`. Give two possible causes.

<details markdown="1"><summary>Answer</summary>

One is that there is no tf chain from the point cloud `frame_id` (for example `lidar_link`) to `odom`, or that a `static_transform_publisher` in the middle or the driver's tf broadcast is not running. The other is that the QoS of the point cloud topic and the subscriber QoS of RViz2 do not match, so the messages themselves are not arriving at all (in this case the error message is closer to "no data is coming" than to "the frame was not found"). If you first check with `ros2 topic hz` whether data is really flowing, it is easier to narrow down which one is the cause.

</details>

### Topic: Convert types with pointcloud_to_laserscan

Topic: Convert a 3D point cloud into a format that a 2D navigation stack can use.

Preparation: `ros-jazzy-pointcloud-to-laserscan` is installed.

Content:

When you want to use the PointCloud2 of a 3D LiDAR as it is for 2D-style obstacle detection or SLAM, the standard way is to put the node of the `pointcloud_to_laserscan` package in between. It picks out only the points in a specified height range, computes the shortest distance for each angle, and republishes it as `LaserScan`.

```bash
ros2 run pointcloud_to_laserscan pointcloud_to_laserscan_node \
  --ros-args \
  -r cloud_in:=/points \
  -r scan:=/scan \
  -p target_frame:=base_link \
  -p min_height:=-0.1 \
  -p max_height:=0.3
```

`min_height`/`max_height` are the height range seen from `base_link`. By narrowing it to near the driving surface, you can ignore unrelated points on the ceiling or on the upper part of walls. How to choose this height depends on the robot's height and the sensor's mounting height, so there is no single correct value. It is safe to look at the point cloud in `rviz2` and decide by eye which range you want to pick up as obstacles for driving.

## Going further

The values of this LiDAR/IMU/PointCloud2 become, as they are, the input of the next article, [22_thinking_about_localization](22_thinking_about_localization.md). Localization algorithms (AMCL, NDT, EKF, and so on) compute "where the robot is now from these sensor values". But if the frame_id, stamp, and covariance of the input sensor values are not correct, the accuracy does not come out however good the algorithm is. The pitfalls covered in this article are also the first places to review when localization does not work well in 22.

Integrating point clouds from several LiDARs/cameras on one robot (extrinsic calibration and multi-sensor fusion) is a theme that fills an article by itself, so we do not cover it here.


## Conclusion

In sensor integration, the worst state is "it looks like it works, but it is actually not connected". Make a habit of checking these three in order: the topic is flowing, the frame_id is correct, and tf is connected. If you do not understand something, ask someone experienced or check the official documentation.

Next is [22_thinking_about_localization](22_thinking_about_localization.md), where we actually estimate the position using the sensor values prepared here.

## References

- [ROS 2 Documentation: Jazzy — sensor_msgs](https://docs.ros.org/en/jazzy/p/sensor_msgs/)
- [ROS 2 Documentation: Jazzy — Using tf2 with sensor data](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Tf2/Tf2-Main.html)
- [livox_ros_driver2 (GitHub)](https://github.com/Livox-SDK/livox_ros_driver2)
- [pointcloud_to_laserscan (GitHub)](https://github.com/ros-perception/pointcloud_to_laserscan)
- [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md)
- [20_ros2_control_overview](20_ros2_control_overview.md)
- [22_thinking_about_localization](22_thinking_about_localization.md)
