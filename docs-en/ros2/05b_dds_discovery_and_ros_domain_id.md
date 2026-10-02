# ROS 2 Lecture 05b: DDS, Discovery, and ROS_DOMAIN_ID

## Introduction

When you finish this article, you can explain how it works: "why does my neighbor's node appear in my `ros2 node list`?" and "why does it stop appearing when I set `ROS_DOMAIN_ID`?"

This is the separate article that the Advanced section of [05_topics](05_topics.md) refers to when it says "we leave the deeper explanation to another article".

**This article is about handling trouble that actually happens when several people are on the same network.** When several people connect to the same network and run ROS 2, the following things happen.

- You started only `turtlesim`, but unknown nodes are listed in `ros2 node list`
- Your `cmd_vel` moves your neighbor's robot
- Your tests pass in your environment, but fail when several people work on the same network

None of these are "bugs". They are the result of DDS discovery working as designed. If you know the mechanism, one command prevents them.

The prerequisite is that you have read up to [05_topics](05_topics.md).

## Lecture goals

- Explain that ROS 2 communication is built on the layers DDS/rmw/rcl
- Explain how discovery finds nodes
- Understand the difference between `ROS_DOMAIN_ID` and `ROS_AUTOMATIC_DISCOVERY_RANGE`, and use each as the situation needs
- Isolate and solve the "I can see other people's nodes" trouble by yourself

## Using this as a course

### Things to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco already set up ([02_environment_setup](02_environment_setup.md) done)
- The `ros-jazzy-demo-nodes-cpp` package (if it is not installed, run `sudo apt install ros-jazzy-demo-nodes-cpp`)
- A screen where you can open two or more terminals
- The `ss` command (the `iproute2` package. It is installed by default on Ubuntu 24.04)
- **If possible, two or more PCs.** With two machines connected to the same network, you can demonstrate "I can see other people's nodes". All tasks in the article can also be done with one machine

### Time allocation guide

- Explaining the layers (DDS/rmw/rcl): 10 min
- Discovery experiment (separating domains): 20 min
- Checking port numbers: 10 min
- `ROS_AUTOMATIC_DISCOVERY_RANGE` experiment: 15 min
- Troubleshooting practice and oral exam: 15 min

### Oral exam

**Q1. Why does `ros2 node list` show other people's nodes? Is it a bug?**

Model answer: It is not a bug. It is the result of DDS discovery working to specification. At startup, DDS announces "I am here" to the network by multicast, and receives announcements from other participants. The default `ROS_AUTOMATIC_DISCOVERY_RANGE` is `SUBNET`, so announcements reach the whole subnet. As a result, nodes of other people on the same LAN are visible to each other. The default of ROS 2 assumes that "several nodes inside one robot connect automatically", so in a situation where several people develop separate robots on one LAN, you need to separate them explicitly.

**Q2. `ROS_DOMAIN_ID` and `ROS_AUTOMATIC_DISCOVERY_RANGE` are both settings to "avoid mixing with other people". Explain the difference.**

Model answer: `ROS_DOMAIN_ID` is a setting that "changes the channel". The UDP port numbers themselves change (the base is `7400 + 250 × domain ID`). Different domains use different ports, so they cannot see each other even on the same network. `ROS_AUTOMATIC_DISCOVERY_RANGE` is a setting that "changes the range that announcements reach". With `LOCALHOST`, announcements do not go out of your own PC. The former is like "using a wireless channel different from others in the same room", and the latter is like "not sending radio waves out of the room at all". Several people do not work inside the same PC, so in practice `LOCALHOST` is more reliable. Using both together is even safer.

**Q3. Explain the range of values you may give to `ROS_DOMAIN_ID`, and the reason.**

Model answer: The maximum by specification is 232. `7400 + 250 × 232 + 11 = 65411`, and above this you would exceed the UDP port number limit of 65535. In practice, however, it is safe to keep it within **0 to 101**. The range that does not collide with the Linux ephemeral port range (`/proc/sys/net/ipv4/ip_local_port_range`, 32768 to 60999 in many environments) is up to `7400 + 250 × 101 = 32650` for domain 101. If you use 102 or higher, a hard-to-reproduce bug can happen: another application happened to take the same port first, so startup fails.

**Q4. You got a report at a study group: "my test passes in my environment, but fails when everyone is working". How do you isolate it?**

Model answer: First, check with `ros2 node list` and `ros2 topic list` whether you can see nodes or topics that you did not start. If you can, suspect DDS crosstalk. Set `ROS_DOMAIN_ID` to a value that does not overlap with others, or set `ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST`, and run again. If it passes then, crosstalk is the cause. It tends to happen when the test uses topics with common names (such as `/chatter` or `/cmd_vel`). As a permanent measure, fix the environment variables on the test runner side (the `drill` script of `ros2-drill` does this).

## Main content

### What you learn: What is under ROS 2

What you learn: Understand the layers DDS / rmw / rcl / rclcpp.

Preparation: Nothing in particular.

Content:

In [05_topics](05_topics.md) we wrote that "DDS runs behind topics". To be a little more exact, ROS 2 has four layers.

```
+----------------------------------------------------+
|  Your code (MinimalPublisher, etc.)                |
+----------------------------------------------------+
|  rclcpp / rclpy  ... API for each language         |
+----------------------------------------------------+
|  rcl             ... language-independent C API    |
+----------------------------------------------------+
|  rmw             ... C interface abstracting DDS   |
+----------------------------------------------------+
|  DDS implementation (Fast DDS / Cyclone DDS / ...) |
+----------------------------------------------------+
```

**Resolving topic names and checking QoS compatibility are done by `rcl` and `rmw`, and the DDS implementation actually sends packets onto the network.** ROS 2 itself has no communication protocol. It borrows an existing industrial middleware called DDS, and lays the ROS concepts (nodes, topics, services) on top of it.

You can check which DDS implementation your environment uses with `ros2 doctor`.

```bash
ros2 doctor --report
```

It appears in the `RMW MIDDLEWARE` section.

```
   RMW MIDDLEWARE
middleware name    : rmw_fastrtps_cpp
```

**The default of Jazzy is Fast DDS (`rmw_fastrtps_cpp`).** It is an implementation by eProsima, and it comes along when you install ROS 2 with apt.

You can also switch to another implementation.

```bash
sudo apt install ros-jazzy-rmw-cyclonedds-cpp
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
```

However, **we do not switch in this course.** We proceed with the default Fast DDS. If you change the implementation, even the fine behavior of discovery and the format of the configuration files change, so it is better not to touch it without a clear reason (for example, wanting to avoid a specific bug).

Note that `ROS_DOMAIN_ID` and `ROS_AUTOMATIC_DISCOVERY_RANGE` are environment variables read by the `rcl` layer, so **they work the same way even if you change the DDS implementation.** The distinction that these are "ROS 2 settings" and not "Fast DDS settings" will help later when you look up settings.

### What you learn: What discovery does

What you learn: Understand how nodes find each other.

Preparation: Two terminals.

Content:

If you start `ros2 run demo_nodes_cpp talker` and type `ros2 node list` in another terminal, `/talker` appears. At this point, **you never specified an IP address or a port number.**

Discovery makes this possible. When a DDS participant starts, it does the following.

1. It sends an announcement to the network by multicast: "I am here. I publish these topics"
2. It waits for the same kind of announcements from other participants
3. When it receives an announcement, it looks at the other side's topic names and QoS, and connects if something matches its own

**QoS is used to decide "if something matches".** That is why the QoS mismatch covered in [05_topics](05_topics.md) shows up as "does not connect". Discovery has succeeded, and the connection decision fails.

Let us try it. Start a talker in terminal 1.

```bash
ros2 run demo_nodes_cpp talker
```

Check in terminal 2.

```bash
ros2 node list
ros2 topic list
```

```
/talker
```

```
/chatter
/parameter_events
/rosout
```

`/parameter_events` and `/rosout` appear even though you did not create them. These are topics that come automatically when you create a node. They are used to notify parameter changes and to deliver logs.

> Column: The result of `ros2 node list` is cached by a background process called `ros2 daemon`. You can see its state with `ros2 daemon status`. If a node remains in the list even after you stopped it, suspect this cache. If you stop it with `ros2 daemon stop`, it is recreated by the next command. In the experiments of this article too, if the result does not change even though you changed an environment variable, stop the daemon first.

### What you learn: Separate channels with ROS_DOMAIN_ID

What you learn: Check that nodes can no longer see each other when you change the domain.

Preparation: Two terminals. Stop the talker from the previous task.

Content:

In terminal 1, specify domain 42 and start a talker.

```bash
export ROS_DOMAIN_ID=42
ros2 run demo_nodes_cpp talker
```

In terminal 2, look from the **same domain**.

```bash
ROS_DOMAIN_ID=42 ros2 node list
```

```
/talker
```

You can see it. Next, look from a **different domain**.

```bash
ROS_DOMAIN_ID=43 ros2 node list
```

Nothing appears. The talker is running, but from domain 43 it is the same as not existing.

**This is the basic form of crosstalk prevention on a shared network.** If each person uses a different domain, they do not interfere with each other even on the same network.

#### What is happening: look at the port numbers

The reality of "different channel" is UDP port numbers. Let us look at them. While the talker is running, check the UDP ports that process has open.

```bash
pid=$(pgrep -x talker)
ss -ulnp | grep "$pid"
```

This is the actual output when run in domain 42 (an excerpt).

```
UNCONN 0  0  0.0.0.0:17900  0.0.0.0:*  users:(("talker",pid=155463,fd=26))
UNCONN 0  0  0.0.0.0:17910  0.0.0.0:*  users:(("talker",pid=155463,fd=25))
UNCONN 0  0  0.0.0.0:17911  0.0.0.0:*  users:(("talker",pid=155463,fd=28))
```

`17900`, `17910`, `17911`. These numbers are decided by the RTPS specification (the DDS wire protocol).

```
base port                = 7400 + 250 x domain ID
for discovery            = base port
for discovery (per-proc) = base port + 10 + 2 x participant number
for data (per-proc)      = base port + 11 + 2 x participant number
```

Let us calculate for domain 42.

```
7400 + 250 x 42 = 17900   <- for discovery (multicast)
17900 + 10 + 0  = 17910   <- for discovery (this process only)
17900 + 11 + 0  = 17911   <- for data (this process only)
```

**It matches the three observed numbers exactly.** If you change the domain ID, this base port shifts by 250 each, so packets of another domain physically do not reach you. It is not that you "cannot see" them. You are "looking at a different port".

#### Range of values you may use

Once you know the formula, you can also derive the upper limit you can specify.

| Domain ID | Base port | Data port | Verdict |
|---|---|---|---|
| 0 | 7400 | 7411 | OK |
| 42 | 17900 | 17911 | OK |
| 101 | 32650 | 32661 | OK (practical upper limit) |
| 232 | 65400 | 65411 | OK (upper limit by specification) |
| 233 | 65650 | 65661 | **Exceeds the UDP port limit of 65535** |

**The maximum by specification is 232.** Then why do we call 101 the "practical upper limit"? Because it collides with the port range that Linux automatically assigns to applications (ephemeral ports).

```bash
cat /proc/sys/net/ipv4/ip_local_port_range
```

```
32768	60999
```

**32768 and above is an area that other applications may use on their own.** The base port of domain 101 is 32650, which is just below this. If you choose 102 or higher, you may hit a hard-to-reproduce bug: another process happened to take the same port first, so the node cannot start.

**In practice, choose from the range 0 to 101.** If you are unsure, decide based on a value unique to each user (user ID, student number, and so on), and you will not collide.

### What you learn: Narrow the range with ROS_AUTOMATIC_DISCOVERY_RANGE

What you learn: Control the range that announcements reach.

Preparation: Two terminals.

Content:

`ROS_DOMAIN_ID` was a setting that "changes the channel". There is another setting that "does not send announcements outside at all".

`ROS_AUTOMATIC_DISCOVERY_RANGE` accepts four values. They are defined in the ROS 2 header (`/opt/ros/jazzy/include/rmw/rmw/discovery_options.h`).

| Value | Meaning |
|---|---|
| `OFF` | Do not do automatic discovery |
| `LOCALHOST` | Only inside your own PC |
| `SUBNET` | The whole same subnet (**default**) |
| `SYSTEM_DEFAULT` | Follow the setting of the DDS implementation |

**The default being `SUBNET` is the cause of the trouble on a shared network.** If you set nothing, your node keeps announcing "I am here" to the whole LAN.

Let us experiment. In terminal 1, set `LOCALHOST` and start a talker.

```bash
ROS_DOMAIN_ID=45 ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST ros2 run demo_nodes_cpp talker
```

From terminal 2, look with the same settings.

```bash
ROS_DOMAIN_ID=45 ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST ros2 node list
```

```
/talker
```

**You can see it, because it is inside the same PC.** `LOCALHOST` is a setting where "inside your own PC connects, but nothing goes outside". As long as you develop on one PC, there is no inconvenience.

If you have two PCs, type `ROS_DOMAIN_ID=45 ros2 node list` on the other one. The node that you could have seen with `SUBNET` (the default) is invisible with `LOCALHOST`. This is the intended effect.

#### With OFF, you see nothing

`OFF` stops discovery itself. If you try it, ROS 2 kindly warns you.

```bash
ROS_DOMAIN_ID=44 ROS_AUTOMATIC_DISCOVERY_RANGE=OFF ros2 node list
```

```
Warning: ROS_AUTOMATIC_DISCOVERY_RANGE=OFF with no ROS_STATIC_PEERS configured.
No discovery mechanism is available. Results will be empty.
Either:
  - Set ROS_STATIC_PEERS to specify peers explicitly, or
  - Change ROS_AUTOMATIC_DISCOVERY_RANGE to LOCALHOST or SUBNET
```

**Even a talker inside the same PC is invisible.** `OFF` is a setting for operation where "you list all communication partners yourself", and what you use then is `ROS_STATIC_PEERS`.

```bash
export ROS_AUTOMATIC_DISCOVERY_RANGE=OFF
export ROS_STATIC_PEERS="192.168.1.10;192.168.1.11"
```

You can specify several, separated by `;`. It is a means for networks where multicast does not pass (some wireless access points and cloud environments). **You do not use it in normal robot development.** Just know the name.

#### Which one should you use

| Situation | Recommended |
|---|---|
| Develop and test on one PC | `ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST` |
| Several people develop separate robots on the same network | A different `ROS_DOMAIN_ID` for each person (0 to 101) |
| Run one robot split across several PCs | The same `ROS_DOMAIN_ID` + `SUBNET` (leave the default) |
| A network where multicast does not pass | `OFF` + `ROS_STATIC_PEERS` |

**If you are unsure, use `LOCALHOST`.** A setup split across several PCs is needed for the first time when you operate a real robot. Until then, `LOCALHOST` causes no trouble.

By the way, the ROS 2 header says this.

> It is intended that the default will be LOCALHOST in future versions of ROS.

It clearly states that **in future ROS 2 the default is planned to be `LOCALHOST`**. Setting `LOCALHOST` now is just getting ahead, and nothing special.

### What you learn: A real example in the test environment

What you learn: Read how it is set in real code.

Preparation: Nothing in particular (just read code).

Content:

The test runner of `ros2-drill`, the C++/ROS 2 practice material, sets both of these environment variables. It is the `ros_env()` function of the `drill` script.

```python
def ros_env():
    env = os.environ.copy()
    # Keep DDS closed so it does not mix with other learners on the same LAN.
    env.setdefault("ROS_AUTOMATIC_DISCOVERY_RANGE", "LOCALHOST")
    env.setdefault("ROS_DOMAIN_ID", str(1 + os.getuid() % 99))
```

Source: `ros2-drill/drill` (`ros_env()`)

Read what the two lines do.

**The first line** is a setting that does not send announcements out of your own PC. Even if several people are on the same network in a course, their tests do not interfere with each other.

**The second line** decides the domain ID from the UNIX user ID. It is `1 + uid % 99`, so it falls in the **range 1 to 99**. This is inside the "0 to 101 is safe" range you saw in the previous task.

```bash
id -u
```

On this machine it was `1000`, so `1 + 1000 % 99 = 11`. Domain 11 is used.

**Why decide from the uid?** So that it does not collide even when different users work on the same PC at the same time. And it matters that "it separates automatically without people setting it by hand". If you tell learners "please set your own domain ID", someone always forgets.

**Also notice that it uses `setdefault`.** If the environment variable is already set, it respects that value. The design is "the default is on the safe side, and you can overwrite it when needed".

This test runs the learner's node and the verification node in the same process, so it does not need to go out to the network in the first place. That is why it loses nothing with `LOCALHOST`. **The principle is to "close communication ranges you do not need".**

## Advanced

### The discovery server option

The discovery so far is the "everyone announces to everyone" method. When nodes increase, the announcement traffic grows with the square of the number of participants, so it becomes a problem at the scale of dozens of nodes.

Fast DDS has a mechanism called the **discovery server**, which lets you change to a form where everyone queries one server. The announcement traffic stays linear.

Most robot systems have not reached this scale at present, so they do not use it. It is something to consider once `ros2 node list` takes several seconds to return. Just remember the name.

### A QoS mismatch and a discovery failure are different things

When "the topic does not connect", there are two kinds of causes. It is quicker if you have a procedure for isolating them.

| Symptom | Cause | How to check |
|---|---|---|
| The other side does not appear in `ros2 node list` | **Discovery failure** | Domain ID, discovery range, network |
| You can see the other side, but nothing appears in `ros2 topic echo` | QoS mismatch, or the publisher's implementation | `ros2 topic info -v` |

**First check whether you can see the other side in `ros2 node list`.** If you can, discovery has succeeded, so changing the domain ID does not solve it however much you try. It is time to suspect QoS ([05_topics](05_topics.md)) or the publisher's implementation ([11_writing_pub_sub_in_cpp](11_writing_pub_sub_in_cpp.md)).

When you write in C++ and "I publish but nothing appears", the most common cause is that **you forgot to keep the return value of `create_publisher()` in a member variable.** rclcpp holds the entities it creates only with `weak_ptr`, so they disappear unless the caller keeps holding them. No error or warning appears. This is covered from the mechanism in [C++ Chapter 6, Smart Pointers](../cpp/06_smart_pointers.md).

### Check that multicast passes

Discovery depends on multicast. If multicast is blocked on the network side, nodes are not found even when the settings are correct.

ROS 2 has a command to check this directly. Receive on one machine and send from another.

```bash
# Receiver (start it first and let it wait)
ros2 multicast receive
```

```bash
# Sender
ros2 multicast send
```

The sender prints this and ends.

```
Sending one UDP multicast datagram...
```

If the receiver shows this, multicast is passing (measured on the same machine).

```
Waiting for UDP multicast datagram...
Received from 10.28.0.217:37857: 'Hello World!'
```

If the receiver stays at `Waiting for ...`, it is not passing. It is time to suspect the network (wireless AP settings, firewall, container network mode).

Remember this as a tool for isolating: **"before suspecting the ROS 2 settings, check whether the network passes"**. There are problems that you cannot solve however much you investigate on the ROS 2 side.

### When you use Docker

If you chose Docker in [02_environment_setup](02_environment_setup.md), with the default bridge network, nodes inside the container and nodes on the host cannot see each other. This is because multicast does not cross the container's network boundary.

If you share the host network with `--network host`, they become visible. In that case, they mix completely with nodes on the host side, so separation with `ROS_DOMAIN_ID` becomes even more important.


## Summary

What this article covered comes down to two environment variables.

```bash
export ROS_DOMAIN_ID=11                        # Change the channel (0 to 101)
export ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST # Do not send announcements outside
```

**We recommend writing them in `~/.bashrc`.** If you set them by hand every time you work, you will forget. When you forget, it does not stop at "my test fails". It can even go as far as "I move my neighbor's robot".

And be ready to explain "why this fixes it". If you remember it as a magic spell, you cannot apply it when a slightly different symptom appears. If you can go back to the port number formula, you can trace the cause when `ros2 node list` is empty.

Next, go back to [06_services](06_services.md) and move on to request/response communication, the counterpart of topics.

## References

- [ROS 2 Documentation: Jazzy — The ROS_DOMAIN_ID](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Domain-ID.html)
- [ROS 2 Documentation: Jazzy — Discovery](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Discovery.html)
- [ROS 2 Documentation: Jazzy — About Quality of Service settings](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html)
- [ROS 2 Documentation: Jazzy — About internal ROS 2 interfaces (rmw)](https://docs.ros.org/en/jazzy/Concepts/Advanced/About-Internal-Interfaces.html)
- [eProsima Fast DDS — Discovery server](https://fast-dds.docs.eprosima.com/en/latest/fastdds/discovery/discovery_server.html)
- `/opt/ros/jazzy/include/rmw/rmw/discovery_options.h` — definition of the four values of `ROS_AUTOMATIC_DISCOVERY_RANGE`
- `/opt/ros/jazzy/include/rcl/rcl/discovery_options.h` — that the default is `SUBNET`, the intent to make it `LOCALHOST` in the future, and the separator of `ROS_STATIC_PEERS`
- [05_topics](05_topics.md) — prerequisite of this article. The basics of QoS
- [02_environment_setup](02_environment_setup.md) — prerequisite when you chose the Docker setup
