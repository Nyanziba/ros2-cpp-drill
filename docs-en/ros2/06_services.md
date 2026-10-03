# ROS 2 Lecture 06: Services

## Introduction

In the previous article, [05_topics](05_topics.md), we covered how to pass continuously flowing data with pub/sub. This time we cover the other means of communication, services. When you finish this article, you can call the turtle with `ros2 service call`, and you can explain in your own words "how to choose between topics and services".

The assumed environment is Ubuntu 24.04 / ROS 2 Jazzy Jalisco.

## Lecture goals / how to proceed

- Audience: people learning this for the first time who have finished [05_topics](05_topics.md)
- Time needed: about 1 to 1.5 hours of self-study
- Prerequisites: you can use the `ros2 topic` commands, and you can start turtlesim
- Goal: understand how services work, and call any service with `ros2 service call`

## Using this as a course

### Things to prepare

- A machine with Ubuntu 24.04 / ROS 2 Jazzy already set up (finished up to [02_environment_setup](02_environment_setup.md))
- The turtlesim package (installed with `sudo apt install ros-jazzy-turtlesim`)

### Oral exam (with model answers)

**Q1. What is the biggest difference between a topic and a service?**

Model answer: A topic is one-way communication in which a subscriber receives the data that a publisher keeps sending. The sender does not care whether a receiver exists. A service is request/response communication. The client sends a request, and the server processes it and returns a response once. A call needs a counterpart (the server), and the client waits until the response comes back.

**Q2. Should the processing that delivers sensor values and the processing that "resets the map" each be implemented as a topic or a service? Give the reason too.**

Model answer: Sensor values are a topic. The values keep changing, and they should keep being delivered whether or not anyone is watching. Resetting the map is a service. A reset is "an operation that happens occasionally and needs a result of whether it succeeded". It does not need to be delivered continuously, and it is enough that one response returns for one request.

**Q3. When you added a turtle in turtlesim with `ros2 service call`, at what timing did the command return control? Why?**

Model answer: Control returns when the server-side processing (creating the turtle) is complete and the response comes back. This is because a service is a communication method that waits in a blocking way from sending the request until the response returns.

### Time allocation guide

- Explaining the concept of services: 10 min
- Operating the `ros2 service` commands: 15 min
- Task of adding turtles with `/spawn`: 15 min
- Discussion of choosing between them and oral exam: 10 min

## Main content

### What is a service

A service is a communication method in which "when you send a request, the server does some processing and returns a response once". While a topic "keeps delivering", a service can be used like a function call where "when you call it, one answer comes back".

The structure is as follows.

- Service server: a node that receives a request, processes it, and returns a response
- Service client: a node that sends a request and waits for the response
- Service type: a set of a request type and a response type. It is defined in a `.srv` file

What matters is that the client waits until the response comes back. This is called blocking. Publishing to a topic finishes immediately whether or not anyone is there. A service call stops the caller's processing until the server responds. This property matters later.

### Start turtlesim

If it is not running yet, prepare two terminals as usual.

```bash
ros2 run turtlesim turtlesim_node
```

```bash
ros2 run turtlesim turtle_teleop_key
```

### See the list of services

In a third terminal, list the running services.

```bash
ros2 service list
```

You see `/clear`, `/kill`, `/reset`, `/spawn`, `/turtle1/set_pen`, `/turtle1/teleport_absolute`, `/turtle1/teleport_relative`, and so on. Where topics had `ros2 topic list`, services have `ros2 service list`. The command system corresponds directly, so it should be easy to remember.

To check the type, use `type`.

```bash
ros2 service type /spawn
```

It returns `turtlesim/srv/Spawn`. As with topics, you can also see the list and the types together with `ros2 service list -t`.

```bash
ros2 service list -t
```

You can also see the relation to nodes with `info`.

```bash
ros2 service info /spawn
```

You can see which node (turtlesim_node) provides this service.

### Check the service type

Before calling `/spawn`, check the content of the request and the response.

```bash
ros2 interface show turtlesim/srv/Spawn
```

The output looks like this.

<!-- measure: env=static reason="turtlesim_node の起動が要り、Docker イメージに turtlesim も無い" -->
```
float32 x
float32 y
float32 theta
string name
---
string name
```

Above the `---` is the request (the values you pass when you call), and below is the response (the values that come back). A topic's `.msg` is only one data structure, but a service's `.srv` is split into two, request and response. This is the difference in structure.

`x`, `y`, and `theta` are the coordinates and heading of the new turtle, and `name` is the name. If you pass an empty string, a name such as `turtle2` is given automatically, and the name actually given comes back in the response `name`.

### Task: Add a turtle

Call `/spawn` to add a turtle.

```bash
ros2 service call /spawn turtlesim/srv/Spawn "{x: 2, y: 2, theta: 0.2, name: ''}"
```

Another turtle appears in the turtlesim window, and a response like the following is shown in the terminal.

<!-- measure: env=static reason="turtlesim_node の起動が要り、Docker イメージに turtlesim も無い" -->
```
requester: making request: turtlesim.srv.Spawn_Request(x=2.0, y=2.0, theta=0.2, name='')

response:
turtlesim.srv.Spawn_Response(name='turtle2')
```

The structure of the command is `ros2 service call <service name> <type> "<request in YAML format>"`. It is almost the same form as `ros2 topic pub` for topics, so if you are unsure, go back to article 05 and compare.

Let us try what happens when you call it again. Call `/spawn` several times without specifying `name`, and check with `ros2 topic list` that topics such as `/turtle3/pose` increase. You can see that a full set of topics such as `pose` and `cmd_vel` appears for each turtle.

Also try calling with the same name specified (it should give an error). If you can read what is happening from the error message, later debugging becomes easier.

### Try `/clear` and `/reset`

Let us also try services whose request is empty.

```bash
ros2 interface show turtlesim/srv/Empty
```

The type itself has no fields, so the request can be empty.

```bash
ros2 service call /clear std_srvs/srv/Empty
```

The trail lines drawn by the turtle disappear. The position and number of turtles do not change.

```bash
ros2 service call /reset std_srvs/srv/Empty
```

This one returns the turtle to the first single turtle at its initial position. Turtles added with `/spawn` also disappear. `/clear` resets the look (drawing), and `/reset` resets the simulation state itself. Both are services with an empty request, but what the server resets is different.

### Choosing between topics and services

As you can tell from what you have touched so far, the criterion is simple.

| Viewpoint | Topic | Service |
|---|---|---|
| Nature of the data | Keeps flowing continuously | A single operation that happens occasionally |
| Direction of communication | One-way (pub → sub) | Round trip (request → response) |
| How it waits | Does not wait (just keeps delivering) | Waits in a blocking way until the response |
| Competition robot example | Sensor values, `cmd_vel`, robot pose | "Reset the map", "run the calibration" |

Something like a sensor value, where "you want the current value and it is always updated", is a topic. On the other hand, something like "reset the map" or "return to the origin", where you ask once and want it executed once with the result (success/failure) returned, is a service. If you are unsure, ask yourself: "is this data that should keep being delivered, or a one-time operation?"

## Advanced

Note that services are blocking. If you implement an operation that takes a long time (such as moving to a navigation goal that takes several seconds to several minutes) as a service, the client freezes until the response returns. For long-running tasks, there is a mechanism called actions, which have feedback and can be canceled, and we cover it in [08_actions](08_actions.md). For now, remember: "services are for short processing, and long processing uses actions".

## Summary

You have touched the two communication methods, topics and services, so you have covered half of the basic means of communication in ROS 2. Next time, in [07_parameters](07_parameters.md), we cover how to change a node's setting values while it is running. If there is anything you do not understand, ask someone experienced or check the official documentation.

### Matching exercise

After you read this chapter, practice with the matching drill exercise.

- `04_service_server` — Write a service server

```bash
./drill run 04
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 Documentation (Jazzy): Understanding services](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Services/Understanding-ROS2-Services.html)
- Previous: [05_topics](05_topics.md)
- Next: [07_parameters](07_parameters.md)
