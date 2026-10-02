# The design philosophy of rclcpp

This document explains why calls such as `create_publisher()` and `spin()` in the exercises of this drill are written the way they are. It does this by actually reading the rclcpp headers and documentation. Do not memorize the API. If you understand what rclcpp values, you can guess from the structure how an unfamiliar API will probably behave.

The intended readers can write C++ but are beginners to intermediate users of ROS 2. Read this after you have worked through the beginner and intermediate exercises 01 to 11, and before you go on to the advanced exercises 12 to 15 (QoS, Executors and callback groups, zero-copy, and composition). If you are unsure about C++ itself, read the [C++ lectures](cpp/README.md) first.

Where the text says "in Jazzy", it describes an implementation detail that may change between versions. Everything else is meant to be fairly general design knowledge about rclcpp. Where I want to avoid stating things too firmly, I hedge with phrases such as "it can be read as". Every file I quote is under `/opt/ros/jazzy/include/`, so you can check it yourself with `grep`.

## Table of contents

1. [Why rclcpp is thin](#1-why-rclcpp-is-thin)
2. [Node is not a god class but a bundle of interfaces](#2-node-is-not-a-god-class-but-a-bundle-of-interfaces)
3. [Everything is "create it, then hold it with shared_ptr"](#3-everything-is-create-it-then-hold-it-with-shared_ptr)
4. [Waiting is the Executor's job](#4-waiting-is-the-executors-job)
5. [Callback groups — the unit of concurrent execution](#5-callback-groups--the-unit-of-concurrent-execution)
6. [Message ownership and zero-copy](#6-message-ownership-and-zero-copy)
7. [QoS is a contract](#7-qos-is-a-contract)
8. [Declare parameters before you use them](#8-declare-parameters-before-you-use-them)
9. [Stop using one node per process](#9-stop-using-one-node-per-process)
10. [Guidelines for using rclcpp efficiently](#10-guidelines-for-using-rclcpp-efficiently)
11. [Going deeper](#11-going-deeper)

---

## 1. Why rclcpp is thin

> Summary: rclcpp is "rcl with a C++ face". The real logic of the ROS concepts (nodes, topics, parameters, and so on) lives in the C-language rcl layer. rclcpp only adds lifetime management with shared_ptr and typing with templates on top of it. Below that there is an abstraction layer called rmw, and below that the actual DDS implementation does the communication.

When you look at `rclcpp::Node` or `rclcpp::Publisher`, it seems that the C++ class hierarchy does everything. But if you follow the implementation of publish in `publisher.hpp`, what it finally does is one call to a C function.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/publisher.hpp
auto status = rcl_publish(publisher_handle_.get(), &msg, nullptr);
```

Look at the includes of `node.hpp` too. The C headers `rcl/error_handling.h` and `rcl/node.h` are included first. rclcpp does not implement its own communication logic. It is a thin wrapper around `rcl` (the ROS Client Library), which is written in C.

Why is there a C layer in between? The official documentation (`Concepts/Basic/About-Client-Libraries.rst`) explains it like this.

> rather than implementing the common functionality from scratch, client libraries make use of a common core ROS Client Library (RCL) interface that implements logic and behavior of ROS concepts that is not language-specific.

If the "logic of the ROS concepts", such as node name resolution and parameter handling, is implemented in only one place in rcl, then rclcpp for C++ and rclpy for Python can share the same behavior. The documentation says there are two benefits: consistency (fixing the logic in rcl is reflected in every language binding) and maintainability (a bug fix is needed in only one place). C was chosen because "it is a language that other languages can wrap easily".

The layers look like this.

```
+----------------------------------------------------+
|  Your code (MinimalPublisher, etc.)                 |
+----------------------------------------------------+
|  rclcpp   ... C++ types, lifetime via shared_ptr    |
|            (Node, Publisher<T>, Executor, ...)      |
+----------------------------------------------------+
|  rcl      ... language-independent C API            |
|            (rcl_publish, rcl_node_init, ...)         |
+----------------------------------------------------+
|  rmw      ... C interface that abstracts DDS        |
|            (rmw_publish, rmw_qos_profile_t, ...)     |
+----------------------------------------------------+
|  DDS implementation (Fast DDS / Cyclone DDS / ...)  |
+----------------------------------------------------+
```

The layers also show up directly in the header dependencies. `rclcpp::QoS` wraps `rmw_qos_profile_t` (`qos.hpp` includes `rmw/qos_profiles.h`), and the Executor waits with `rcl/wait.h` (the includes of `executor.hpp`).

With this view, when you are unsure about an rclcpp API, you can guess that "it is probably just a C++ wrapper around the matching rcl function". The parts of rclcpp that feel thick (templates, callback groups, the Executor) are layers added for C++-specific "lifetime management" and "type safety". They are not ROS concepts themselves.

---

## 2. Node is not a god class but a bundle of interfaces

> Summary: `rclcpp::Node` looks like an "everything" class, but it is really a facade that bundles 11 small interfaces such as `NodeBaseInterface`, `NodeTopicsInterface`, and `NodeParametersInterface`. Free functions such as `create_publisher()` require only these interfaces. So the same mechanism can be reused by classes that do not inherit from `Node` (components and lifecycle nodes).

If you look at the private members of the `Node` class in `node.hpp`, the class does not hold the real data itself. It holds a `shared_ptr` to each interface.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp
rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_base_;
rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr node_topics_;
rclcpp::node_interfaces::NodeServicesInterface::SharedPtr node_services_;
rclcpp::node_interfaces::NodeTimersInterface::SharedPtr node_timers_;
rclcpp::node_interfaces::NodeParametersInterface::SharedPtr node_parameters_;
rclcpp::node_interfaces::NodeClockInterface::SharedPtr node_clock_;
rclcpp::node_interfaces::NodeGraphInterface::SharedPtr node_graph_;
// Others: NodeLogging / NodeTimeSource / NodeTypeDescriptions / NodeWaitables
```

In `/opt/ros/jazzy/include/rclcpp/rclcpp/node_interfaces/` they are lined up, one responsibility per file. Their roles are what the names say. Here are the main ones.

| Interface | Responsibility |
| --- | --- |
| `NodeBaseInterface` | Node name, namespace, the raw `rcl_node_t` handle, and callback group management |
| `NodeTopicsInterface` | Creating and registering Publishers / Subscriptions (`node_topics.hpp`) |
| `NodeServicesInterface` | Creating and registering Services / Clients |
| `NodeParametersInterface` | Declaring, getting, and changing parameters (the real implementation behind `declare_parameter` and others) |
| `NodeGraphInterface` | Graph information equivalent to `ros2 node info` |
| `NodeWaitablesInterface` | Registering generic entities that the Executor can wait on directly (`Waitable`) |

The comment at the top of `node.hpp` introduces `Node` like this.

> Node is the single point of entry for creating publishers and subscribers.

It is the "single entry point", but it is not the "body of the implementation". If you follow the implementation of `create_publisher()` (`node_impl.hpp` / `create_publisher.hpp`), `Node` creates nothing by itself. It hands everything over to `node_topics_interface->create_publisher(...)`.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/create_publisher.hpp (abridged)
auto node_topics_interface = rclcpp::node_interfaces::get_node_topics_interface(node_topics);
auto pub = node_topics_interface->create_publisher(topic_name, factory, actual_qos);
node_topics_interface->add_publisher(pub, options.callback_group);
return std::dynamic_pointer_cast<PublisherT>(pub);
```

Note that `rclcpp::create_publisher<MessageT>(...)` (the free function that `Node::create_publisher()` calls internally) requires only "something that has a method called `get_node_topics_interface()`". The `NodeTopics` class in `node_topics.hpp` is also a thin implementation. It holds no real data and keeps only raw pointers to `NodeBaseInterface*` and `NodeTimersInterface*`.

This design of "requiring only the interfaces you need" pays off for lifecycle nodes (`rclcpp_lifecycle::LifecycleNode`) and components. You do not have to inherit from `rclcpp::Node`. If a class holds the same `NodeBaseInterface` and the others inside, it can reuse `create_publisher()` / `create_subscription()` as they are. "Inheriting from `Node`" is not the only correct answer. The essence is "making a class that has the set of interfaces you need" (`NodeInterfaces<...>` in `node_interfaces/node_interfaces.hpp` is exactly a helper for "bundling only what you need"). This split directly helps in exercise 15 (composition) (see chapter 9).

---

## 3. Everything is "create it, then hold it with shared_ptr"

> Summary: `create_publisher()` and similar functions return a `shared_ptr`. And rclcpp itself (the callback group) holds those entities only through `weak_ptr`. Unless the caller keeps holding the return value, the object disappears. The first stumbling block for beginners, "it does not work if I forget to assign the return value to a member variable", is not an accident. It follows necessarily from this ownership model.

In the declarations in `node.hpp`, `create_publisher` / `create_subscription` / `create_wall_timer` / `create_service` / `create_client` all return `std::shared_ptr<...>`. You probably expected that much. What matters is that the inside of rclcpp does **not hold this Publisher with a strong reference**. In the implementation of `callback_group.hpp`, the registered Publishers / Subscriptions / Timers / Services / Clients are all kept as vectors of `weak_ptr`.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/callback_group.hpp
for (auto & weak_ptr : vect_ptrs) {
  auto ref_ptr = weak_ptr.lock();
  ...
}
```

The signatures of `add_publisher()` and similar functions take a `SharedPtr` as an argument (lines 271 to 287), but inside they convert it to a `weak_ptr` and store that. If you do not save the return value of `create_publisher()` anywhere, the reference count becomes 0 the moment you leave the function, and `weak_ptr::lock()` always returns `nullptr`. There is no error. Nothing just happens. The README of exercise 01 lists the following as the first of its "Common pitfalls".

> Always assign the return value of `create_publisher()` / `create_wall_timer()` to a member variable. If you take it in a local variable, it is destroyed when you leave the constructor, and nothing happens.

This is a logical consequence of the weak_ptr design. It is not a "careless mistake". It is just the natural C++ behavior that "something nobody owns disappears".

The same structure appears in the relationship between a node and an Executor. `Executor::add_node()` takes a `std::shared_ptr<rclcpp::Node>` (`executor.hpp`), but the internal collection that actually stores them, in `executor_entities_collector.hpp`, is defined like this.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/executors/executor_entities_collector.hpp
using NodeCollection = std::set<
  rclcpp::node_interfaces::NodeBaseInterface::WeakPtr,
  std::owner_less<rclcpp::node_interfaces::NodeBaseInterface::WeakPtr>>;
NodeCollection weak_nodes_ RCPPUTILS_TSA_GUARDED_BY(mutex_);
```

The Executor does **not own** nodes. Keeping a node alive is the job of a local variable in `main()`, or of a `std::vector<rclcpp::Node::SharedPtr>` in a test helper. The chain of ownership looks like this.

```
main() / test code
   │  shared_ptr (strong reference, the only owner)
   ▼
rclcpp::Node (your node)
   │  shared_ptr (creates entities via NodeTopicsInterface)
   ▼
Publisher<T> / Subscription<T> / TimerBase / ...
   ▲
   │  weak_ptr (only checks whether it is alive; does not own)
CallbackGroup ◀───── weak_ptr ───── Executor
```

The arrows that "hold strongly" always go from the top (your code) to the bottom (the rclcpp entities). rclcpp itself is designed to hold only weakly what it created. This is probably also the standard practice with shared_ptr for preventing circular references (a Node holds a Publisher, the Publisher's callback refers to the Node, and so on).

---

## 4. Waiting is the Executor's job

> Summary: A Publisher or a Subscription has no "waiting" logic of its own for messages. Waiting is the job of the Executor. It registers entities in an rcl wait set, blocks in `rcl_wait`, takes out only the entities that are ready, and runs them. The `spin` family of functions differ slightly in "what" and "how much" they process.

In ROS 2, messages do not pile up in a queue on the rclcpp side. They stay in the middleware (DDS). The official documentation (`Concepts/Intermediate/About-Executors.rst`) explains it like this.

> Rather than queuing messages at the client library layer, messages remain in the middleware until processing. An Executor uses a "wait set" mechanism with binary flags per queue to detect available messages and expired timers.

The Executor's job is to wait efficiently for things that "may not have arrived yet", and to run them when they do arrive. The protected members of `executor.hpp` list this flow directly as functions.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/executor.hpp
void collect_entities();                       // (1) Collect the entities to wait on
void wait_for_work(std::chrono::nanoseconds);  // (2) Build the wait set and block in rcl_wait
bool get_next_ready_executable(AnyExecutable &); // (3) Take out one ready entity
```

The comment on `wait_for_work()` says "Builds a set of waitable entities, which are passed to the middleware. After building wait set, waits on middleware to notify." The real thing is `rcl_wait_set_t` in `rcl/wait.h` (`/opt/ros/jazzy/include/rcl/rcl/wait.h`). It is a mechanism that receives binary flags, "ready / not ready", from below in DDS. `spin()` just keeps running this loop of (1) → (2) → (3) → run.

`spin` has several variants, and "how far to process" differs between them (from the documentation comments in `executor.hpp`).

| Function | What it does | Does it block? |
| --- | --- | --- |
| `spin()` | Repeats (1) to (3) in an infinite loop | Yes (until `cancel()`) |
| `spin_once(timeout)` | Runs **only one ready item** | For up to `timeout` |
| `spin_some(max_duration)` | Runs all items that are already ready, but does not pick up items that become ready during the run | Basically no |
| `spin_all(max_duration)` | Repeats "collect, then run" until `max_duration` is used up (also picks up items that appear during the run) | For up to `max_duration` |
| `spin_until_future_complete(future, timeout)` | Waits for the future to complete while repeating the equivalent of `spin_once` | For up to `timeout` |

The "only one" limit of `spin_once` becomes a problem when you put several Subscriptions and timers on one Executor. The `spin_until()` in the shared test helper `tools/drill_harness.hpp` of this drill chooses `spin_all`, with the following comment.

```cpp
// tools/drill_harness.hpp
// spin_once processes only one item per call, so with several subscriptions
// only the one registered first keeps running and the other starves.
// spin_all processes "everything that can be processed now".
exec.spin_all(20ms);
```

Suppose the learner's node timer and the test's probe node Subscription are on the same `SingleThreadedExecutor`. If you keep calling `spin_once`, one of them can keep getting priority and the other can be hard to run. This is "starvation". With `spin_all`, it handles everything that is ready in that round without missing any, and then returns. So it suits tests that put several nodes on one Executor and want to check the behavior of both.

Note that `spin_until_future_complete()` is convenient, but it easily conflicts with the callback group constraints (see chapter 5 and exercise 13).

---

## 5. Callback groups — the unit of concurrent execution

> Summary: A callback group is the "unit of callbacks that may run at the same time". The default `MutuallyExclusive` runs only one at a time within the same group, so it is safe. But if you build something that "waits for another callback to finish" inside the group, it deadlocks. `Reentrant` allows concurrent execution, but in exchange the responsibility for thread safety moves to the author.

Every callback (for a Subscription, a Timer, a Service, the completion of a Client, and so on) belongs to some `CallbackGroup`. `callback_group.hpp` defines the types of group like this.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/callback_group.hpp
enum class CallbackGroupType { MutuallyExclusive, Reentrant };
```

- **MutuallyExclusive** (default): Callbacks of the same group never run at the same time, however many threads the Executor has.
- **Reentrant**: Several callbacks of the same group (including concurrent runs of the same callback) can run at the same time on several threads.

`MutuallyExclusive` is a safety measure that prevents "several callbacks touching and breaking the state of the same node at the same time". But this safety measure itself can cause a deadlock. Exercise 13 (`exercises/13_executors/README.md`) demonstrates this.

If you synchronously wait for the response of a service client in the same group, inside a subscriber callback (`future.wait_for(2s)`), this cycle arises.

- `trigger_callback` is waiting for the "completion handling of the client" to run and fill the `future`
- The "completion handling of the client" belongs to the same `MutuallyExclusive` group as `trigger_callback`, so the Executor cannot run it until `trigger_callback` finishes

This is not solved by a `SingleThreadedExecutor`, and it is not solved by adding threads to a `MultiThreadedExecutor` unless you split the groups. The reason is that you have not given the Executor the information that "different groups may run at the same time". The solution is to put the subscriber and the client in **different** `MutuallyExclusive` groups, and also to give the `MultiThreadedExecutor` the capacity to process several callbacks at once. Since the two are unrelated tasks, there is no need to make them `Reentrant`. It is enough that each is exclusive inside its own group.

It helps to know these three Executor implementations (`executors/` also has the experimental `events_cbg_executor`).

| Executor | Threads | Characteristics |
| --- | --- | --- |
| `SingleThreadedExecutor` | 1 | What `rclcpp::spin()` uses. It is enough for 01 to 07. Even if you split the groups, it cannot run callbacks at the same time. |
| `MultiThreadedExecutor` | Several (default 0 = number of CPU cores, at least 2) | If the groups are split, it can run callbacks at the same time on different threads. This is the one used in exercise 13. |
| `StaticSingleThreadedExecutor` | 1 | Builds the entity list only once when spin starts (less overhead). It still has one thread, so it does not solve exercise 13. |

Jazzy also has `rclcpp::experimental::executors::EventsExecutor` (`experimental/executors/events_executor/events_executor.hpp`). As the `experimental` namespace says, its API is not yet stable. The header comments suggest that it receives event notifications directly from the rmw layer instead of rebuilding the wait set every time. This document only introduces it.

---

## 6. Message ownership and zero-copy

> Summary: `publish()` has a version that takes a value and a version that takes a `unique_ptr`. The former always makes one copy. The latter delivers with 0 copies if the conditions are met. The `IntraProcessManager` bridges the two. It is faster because it does not go through DDS, and the bigger the message, the bigger the effect.

The overloads of `publish()` in `publisher.hpp` show this difference clearly.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/publisher.hpp (abridged)
// unique_ptr version: takes the ownership as it is
void publish(std::unique_ptr<T, ROSMessageTypeDeleter> msg)
{
  if (!intra_process_is_enabled_) { this->do_inter_process_publish(*msg); return; }
  this->do_intra_process_publish(std::move(msg));  // Pass it on without copying
}

// By-value version: copies first, then delegates to the unique_ptr version
void publish(const T & msg)
{
  if (!intra_process_is_enabled_) { this->do_inter_process_publish(msg); return; }
  // Otherwise we have to allocate memory in a unique_ptr and pass it along.
  auto unique_msg = this->duplicate_ros_message_as_unique_ptr(msg);
  this->publish(std::move(unique_msg));
}
```

The by-value version "allocates memory and copies" before it delegates to the unique_ptr version. The caller's `msg` stays alive as a local variable, so there is no way to avoid the copy. The unique_ptr version, on the other hand, receives the ownership itself with `std::move`, so it can store that memory as it is in the intermediate buffer.

The real thing behind this "intermediate buffer" is `rclcpp::experimental::IntraProcessManager` (`experimental/intra_process_manager.hpp`). According to the comment in the header,

> A singleton instance of this class is owned by a rclcpp::Context and a rclcpp::Node can use an associated Context to get an instance of this class. Nodes which do not have a common Context will not exchange intra process messages because they do not share access to the same instance of this class.

`IntraProcessManager` is a singleton, **one per `Context` (in most cases, one per process)**. Publishers and Subscriptions find each other by registering here. A node in another process does not share the `Context`, so it can never be a target of intra-process communication.

Here are the conditions under which zero-copy works, following the summary in exercise 14 (`exercises/14_zero_copy/README.md`).

**Conditions that work (all must be met)**

- The Publisher and the Subscription are in the same process
- Both nodes are created with `rclcpp::NodeOptions().use_intra_process_comms(true)`
  (**the default is `false`**. In `node_options.hpp`: `bool use_intra_process_comms_ {false};}`)
- The QoS History is `KeepLast` / `KeepAll` (the form that has a depth)
- The publish side passes the ownership with `std::move(unique_ptr)`

**Conditions that do not work (it goes back to copying)**

- The Publisher and the Subscription are in different processes (serialization over DDS is needed)
- `use_intra_process_comms` is not enabled. **Just putting them in the same process does not work**
  (composition does not enable it automatically. See chapter 9)
- You pass a value to publish (`publish(message)`)
- There are several subscribers, and some of them receive in a modifiable form (`UniquePtr` / `SharedPtr`)
  (a copy is made for that part, to keep the single ownership)

Even if you make the subscriber's argument `const T &`, no copy happens (measured in Jazzy. The address of the message is the same on the sending side and the receiving side). `any_subscription_callback.hpp` does not support the receive-by-value form in the first place, so the statement "receiving by value makes a copy" is not accurate. Choose the type by **how you want to handle the message** (only read it on the spot / keep it / modify it).

The bigger the message (such as `sensor_msgs::msg::Image` or `PointCloud2`, which reach hundreds of KB to several MB), the more this difference matters. Passing by value copies the whole message every time, so the number of copies grows linearly with the number of stages. With `std::move(unique_ptr)` the number of copies stays 0. `CameraNode` in `/opt/ros/jazzy/include/intra_process_demo/image_pipeline/camera_node.hpp` publishes camera images with this technique.

`publisher.hpp` has one more API, `borrow_loaned_message()` (it returns `LoanedMessage<T>` from `loaned_message.hpp`). When the RMW implementation can lend memory for messages directly, it skips the heap allocation itself. According to the comment on `LoanedMessage`, it has a fallback: "if the middleware supports loaning, use it, and if not, allocate normally with the given allocator". You can check whether loaning is supported with `PublisherBase::can_loan_messages()` (`publisher_base.hpp`). This is a zero-copy mechanism at the level of the DDS implementation, separate from the zero-copy of intra-process communication.

---

## 7. QoS is a contract

> Summary: QoS (Quality of Service) is a set of "agreements about communication quality" that comes from DDS. If the QoS of a Publisher and a Subscription do not match, the connection is not established. `rclcpp::QoS` decides compatibility with an asymmetric relation between "request" and "offer".

When topic communication in ROS 2 does not connect, the cause is often a QoS mismatch. This is not so much a bug as the design itself. The official documentation (`Concepts/Intermediate/About-Quality-of-Service-Settings.rst`) explains the relation between a Subscription and a Publisher like this.

> Subscriptions request a QoS profile that is the "minimum quality" that it is willing to accept, and publishers offer a QoS profile that is the "maximum quality" that it is able to provide.

A Subscription **requests** a minimum line, "I do not accept anything worse than this". A Publisher **offers** an upper limit, "I can provide this much quality". The connection is established only when, for all requested policies, the offering side is at least as strict as the requesting side.

There are 7 main policies (from the same document).

| Policy | Choices | Meaning |
| --- | --- | --- |
| History | KeepLast(N) / KeepAll | Keep only the last N items, or keep all of them |
| Depth | A number | The queue depth when History is KeepLast |
| Reliability | Reliable / BestEffort | Deliver even by resending, or allow dropping |
| Durability | TransientLocal / Volatile | Keep past samples for Subscriptions that come later, or not |
| Deadline | A time | The upper limit of the interval between consecutive publishes |
| Lifespan | A time | How long a message is valid between publish and receive |
| Liveliness | Automatic / ManualByTopic | Check liveliness automatically, or require explicit assertions |

In `rclcpp/qos.hpp` you set these as a method chain on the `QoS` class (from `qos_nodes.cpp` of exercise 12).

```cpp
// exercises/12_qos/src/qos_nodes.cpp
rclcpp::QoS qos(rclcpp::KeepLast(1));
qos.transient_local();
qos.reliable();
```

The compatibility rules are easy to follow if you think "can the stricter side include the looser side?".

- **Reliability**: A Reliable Subscription does not connect to a BestEffort Publisher. In the other direction, a BestEffort Subscription connects to a Reliable Publisher.
- **Durability**: A TransientLocal Publisher can satisfy a Volatile Subscription, but the reverse (a Volatile Publisher satisfying a TransientLocal request) cannot.
- **Deadline / Liveliness lease**: They are compatible if the Subscription's value is at least as loose as the Publisher's value.
- **Liveliness**: A Subscription that requests ManualByTopic does not connect to an Automatic Publisher.

`LatchedPublisher` / `LatchedSubscriber` in exercise 12 use `KeepLast(1) + transient_local + reliable`. This is the so-called "latched topic" pattern. A TransientLocal Publisher keeps the last value it published, so even a Subscription that starts later can receive the latest setting.

`rclcpp::QoS` also has ready-made profiles that give names to commonly used combinations. They are actually defined, with comments, in `qos.hpp`.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/qos.hpp
/**
 * Sensor Data QoS class
 *    - History: Keep last, Depth: 5,
 *    - Reliability: Best effort, Durability: Volatile, ...
 */
class RCLCPP_PUBLIC SensorDataQoS : public QoS { ... };
```

`SensorDataQoS` is BestEffort with depth 5. It is meant for topics such as cameras and lasers, where "the latest data matters even if you drop a little". Likewise there are `ParametersQoS` (depth 1000, Reliable) and `ServicesQoS` (depth 10, Reliable), and the default values are defined as constants in `rmw/qos_profiles.h`. Before you build a QoS from scratch, it is good to check whether a similar ready-made profile exists.

You can also use `rclcpp::qos_check_compatible(pub_qos, sub_qos)` (`qos.hpp`) to check compatibility from code before you actually connect. It returns one of three levels, `Ok` / `Warning` / `Error`, and the header states that it returns `Warning` when values that are not fixed until run time, such as "system default", are mixed in.

---

## 8. Declare parameters before you use them

> Summary: In rclcpp, you cannot call `get_parameter()` unless you have first declared the type and the default value with `declare_parameter()`. This constraint prevents accidents where an unexpected type or an unexpected parameter slips in at run time. There are two ways to receive values: polling (read every time) and event notification (called only when the value changes).

The documentation comments of the `get_parameter()` family of methods in `node.hpp` mention `ParameterNotDeclaredException` again and again. If you try to read a parameter that is not declared, an exception is thrown by default. The official documentation (`Concepts/Basic/About-Parameters.rst`) explains it like this.

> a node needs to declare all of the parameters that it will accept during its lifetime.

When you declare, you specify the type, the default value, and a `ParameterDescriptor` (metadata such as a description). By default you also cannot change the type of a declared parameter at run time. This lets you detect accidents such as "a typo in the parameter name" or "I meant a string but it was overwritten with a number" early, as run-time errors.

`MinimalParam` in exercise 06 (`exercises/06_parameters/README.md`) is the most basic form of this pattern. It calls `get_parameter()` again **every time inside** the timer callback. The point of the official tutorial is to read it again each time you use it, not to read once and cache. This is the "polling" style. The implementation is simple, but it has the cost of reading every time even when nothing has changed.

The other style is the "event notification" style that uses `ParameterEventHandler` in exercise 07 (`exercises/07_param_events/README.md`). In `parameter_event_handler.hpp`, you can see that this class subscribes to the `/parameter_events` topic internally. A callback registered with `add_parameter_callback("an_int_param", cb)` is called only when that parameter actually changes. The header comment states this warning.

> Note: the object returned from add_parameter_callback must be captured or the callback will [be unregistered].

If you do not keep the return value (`ParameterCallbackHandle::SharedPtr`) in a variable, the callback is unregistered right after you register it. This is another example of the design from chapter 3, "it disappears unless you hold the return value as a shared_ptr". The README of exercise 07 lists this point as "the biggest trap this time".

Here is a guide for which to use.

- **Polling (exercise 06)** — When you already have a loop that runs periodically and you only want to use the latest value along the way. The implementation is simple and no lifetime management is needed.
- **Event notification (exercise 07)** — When you want to run something the moment a parameter changes. If nothing changes, the callback is not called and there is no wasted reading. On the other hand, you take on the added responsibility of keeping the `ParameterEventHandler` and the `ParameterCallbackHandle` alive.

`/parameter_events` is a global topic, so another node can also watch the parameter changes of a node. It can be read that `ParameterEventHandler` is implemented as a topic subscription, not per node, because of this generality.

---

## 9. Stop using one node per process

> Summary: A node registered with `RCLCPP_COMPONENTS_REGISTER_NODE` can be loaded dynamically into a component container and run in the same process as other nodes, instead of being started as its own process with `ros2 run`. For this to work, the node's constructor must follow the convention of taking an `rclcpp::NodeOptions`.

`composable_talker.cpp` in exercise 15 has this shape.

```cpp
// exercises/15_composition/src/composable_talker.cpp
ComposableTalker::ComposableTalker(const rclcpp::NodeOptions & options)
: Node("composable_talker", options), count_(0) { /* ... */ }

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker)
```

The definition of `RCLCPP_COMPONENTS_REGISTER_NODE` (`rclcpp_components/rclcpp_components/register_node_macro.hpp`) states in a comment the conditions for classes that can be registered.

> Valid arguments for NodeClass shall have a constructor that takes a single argument that is a `rclcpp::NodeOptions` instance, and a method of the signature `rclcpp::node_interfaces::NodeBaseInterface::SharedPtr get_node_base_interface`. Note: NodeClass does not need to inherit from `rclcpp::Node`, but it is the easiest way.

Here too the design from chapter 2, "Node is just a facade that has the interfaces you need", matters. You **do not need** to inherit from `rclcpp::Node` to register a component. The comment says clearly that it is enough to have `get_node_base_interface()`. Still, inheriting is the easiest way in practice, so most components have the same shape as this exercise.

Why does the constructor require `NodeOptions`? The component container (`rclcpp_components::ComponentManager`) calls `dlopen` on a shared library at run time and creates the instance through `NodeFactoryTemplate<NodeClass>`. The only argument it can pass at that point is one `NodeOptions`, and every setting the node needs is packed into it. If you look at the default-value comments in `node_options.hpp`, you can see which items are carried.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node_options.hpp (excerpt from the default-value comments)
//   - parameter_overrides = {}
//   - use_intra_process_comms = false
//   - automatically_declare_parameters_from_overrides = false
```

Specifically, it includes command-line arguments (`arguments()`), parameter overrides (`parameter_overrides()`), and `use_intra_process_comms()`, which we used in exercise 14. It is no accident that the purpose of components, "packing several nodes into the same process", goes especially well with zero-copy communication through `use_intra_process_comms(true)`. For nodes in the same process, the `IntraProcessManager` of chapter 6 can be used as it is.

However, **making something a component does not make it zero-copy.** As the default-value comment above shows, `use_intra_process_comms` stays `false`. Unless you enable it explicitly, communication goes through DDS even if the nodes are in the same process (in a local measurement too, with the same process and the default options, the address of the message does not match between the sender and the receiver). The official `composition_demo_launch.py` does not enable it either, so the talker / listener started by that launch communicate through DDS even though they are in the same process. To enable it, you pass it into `NodeOptions` when you load the node into the container, like this.

- CLI: `ros2 component load /ComponentManager <pkg> <plugin> -e use_intra_process_comms:=true`
- launch: `ComposableNode(..., extra_arguments=[{'use_intra_process_comms': True}])`

In other words, making something a component only **meets the precondition** of "putting it in the same process". Zero-copy itself is a separate opt-in.

If you break the convention of passing the `NodeOptions` received in the constructor **as it is** to `Node("...", options)`, you get a confusing bug. Everything is fine when you start the node alone with `ros2 run`, but as soon as you put it in a component container, `use_intra_process_comms` and parameter overrides do not take effect. The README of exercise 14 warns that "if you write `Node("...")`, `use_intra_process_comms` is not passed on, a copy always happens, and the test fails". That warning is based on the necessity of this path.

---

## 10. Guidelines for using rclcpp efficiently

> Summary: These are rules of thumb that work in practice, based on the structure so far. I added the reasons, so that you can recall "why you should do it" from the structure.

1. **Decide QoS at the start of the design.** If you change only one side later, they silently stop connecting because of the compatibility rules in chapter 7. Decide by purpose first: `SensorDataQoS` for sensors, `transient_local` for distributing settings.
2. **Do not put heavy or blocking work inside a callback.** As in chapter 4, if one callback runs long, its Executor stops (the same group, or everything in the case of a `SingleThreadedExecutor`).
3. **If you wait synchronously for something else to complete inside a callback, design your callback groups.** The deadlock in chapter 5 happens when you "wait for each other in the same group". Put the waiting side and the waited-for side in different groups, and give the `MultiThreadedExecutor` spare threads.
4. **Do not call `spin_until_future_complete()` from inside a callback.** If you try to start spinning on the same node from a callback that is already running inside an Executor, you get an exception or a deadlock. Wait with `future.wait_for()`, or rethink the design.
5. **Write large messages in a form where zero-copy works.** Use `publish(std::make_unique<T>(...))` instead of `publish(msg)`, and receive with `ConstSharedPtr` on the subscriber side (chapter 6).
6. **Make `msg::X::ConstSharedPtr` the basic form of the subscription callback argument.** When intra-process communication is enabled you get the benefit of zero-copy as it is, and when it is disabled the cost does not increase.
7. **Always keep the return values of `create_publisher()` and similar functions in member variables.** As in chapter 3, rclcpp holds them only through weak_ptr, so if you do not hold them they disappear.
8. **Prefer `create_wall_timer()` to `rclcpp::Rate`.** `Rate::sleep()` blocks the calling thread. If you use it on an Executor thread, other callbacks cannot be processed. `create_wall_timer()` takes part in the Executor's waiting, so they can coexist.
9. **Use `RCLCPP_INFO_THROTTLE` and similar macros for high-frequency logs.** The `_THROTTLE` macros in `logging.hpp` log only at the given interval, which prevents the logging itself from putting pressure on processing.
10. **Always "declare parameters before you use them", and state the type explicitly.** As in chapter 8, accessing an undeclared parameter throws an exception.
11. **Distinguish whether you want to react to a parameter change or just read it along with periodic work.** For the former, use `ParameterEventHandler`. For the latter, calling `get_parameter()` each time is enough.
12. **If you expect to use composition, write the constructor to take `NodeOptions` from the start.** Adding it later is a hassle, as in chapter 9.
13. **Choose the Executor type by "whether you need concurrent execution".** If you do not, a `SingleThreadedExecutor` (`rclcpp::spin()`) is enough.
14. **Do not think `spin_once` is "something that processes everything in one call".** As in chapter 4, it processes only one. If you want to process several entities reliably, consider `spin_some` / `spin_all` (see also the choice in `tools/drill_harness.hpp`).
15. **When in doubt, read the header.** The rclcpp headers have rich Doxygen comments, and they often even say "what this function calls".

---

## 11. Going deeper

> Summary: This is a list of where the primary sources quoted in this document are, and a table of how they match the exercises of this drill. If you doubt an implementation, first look at the headers and documents listed here.

### Headers (under `/opt/ros/jazzy/include/`)

| Topic | Main files |
| --- | --- |
| Node and the interface split | `rclcpp/rclcpp/node.hpp`, `rclcpp/rclcpp/node_interfaces/*.hpp` |
| Creating Publishers / Subscriptions | `rclcpp/rclcpp/create_publisher.hpp`, `create_subscription.hpp`, `node_interfaces/node_topics.hpp` |
| Callback groups | `rclcpp/rclcpp/callback_group.hpp` |
| Executors in general | `rclcpp/rclcpp/executor.hpp`, `executors/single_threaded_executor.hpp`, `executors/multi_threaded_executor.hpp`, `executors/static_single_threaded_executor.hpp` |
| The experimental EventsExecutor | `rclcpp/rclcpp/experimental/executors/events_executor/events_executor.hpp` |
| The publish implementation of Publisher | `rclcpp/rclcpp/publisher.hpp`, `publisher_base.hpp` |
| Zero-copy / intra-process communication | `rclcpp/rclcpp/experimental/intra_process_manager.hpp`, `loaned_message.hpp` |
| QoS | `rclcpp/rclcpp/qos.hpp`, `rmw/rmw/qos_profiles.h` |
| Parameters | `rclcpp/rclcpp/node_interfaces/node_parameters_interface.hpp`, `parameter_event_handler.hpp` |
| Components | `rclcpp_components/rclcpp_components/register_node_macro.hpp`, `node_options.hpp` |
| rcl (the C layer) | `rcl/rcl/node.h`, `rcl/rcl/wait.h` |

### Official documentation and design documents

- Concepts / Basic / About-Client-Libraries — the relation between rclcpp, rclpy, and rcl (`https://docs.ros.org/en/jazzy/Concepts/Basic/About-Client-Libraries.html`)
- Concepts / Intermediate / About-Executors — the Executor and the wait set (`https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Executors.html`)
- Concepts / Intermediate / About-Quality-of-Service-Settings — the list of QoS settings and compatibility (`https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html`)
- Concepts / Basic / About-Parameters — declaring parameters and parameter services (`https://docs.ros.org/en/jazzy/Concepts/Basic/About-Parameters.html`)
- Concepts / Basic / About-Discovery — automatic discovery with DDS (`https://docs.ros.org/en/jazzy/Concepts/Basic/About-Discovery.html`)
- Tutorials / Demos / Setting up efficient intra-process communication — a tutorial on intra-process communication (in Jazzy it is placed under "Tutorials/Demos", not "Concepts") (`https://docs.ros.org/en/jazzy/Tutorials/Demos/Intra-Process-Communication.html`)
- design.ros2.org / articles / intraprocess_communications — the background of the IntraProcessManager design (this is the description from the time of the proposal, and for implementation details the Jazzy headers are more accurate) (`https://design.ros2.org/articles/intraprocess_communications.html`)
- How-To-Guides / Using callback groups (`https://docs.ros.org/en/jazzy/How-To-Guides/Using-callback-groups.html`)

If you cannot open `docs.ros.org`, you can get the `.rst` source of the pages above in the form `https://raw.githubusercontent.com/ros2/ros2_documentation/jazzy/source/<path>.rst` (this document also checked its content that way).

### Match with the exercises

| Chapter | Matching exercise |
| --- | --- |
| 1. Why rclcpp is thin | Background for all exercises |
| 2. Node is a bundle of interfaces | 15 (composition) |
| 3. Create it, then hold it with shared_ptr | 01, 02, 05, 07 (keeping the return value) |
| 4. Waiting is the Executor's job | All exercises (the reason for `spin_all` in the test helper) |
| 5. Callback groups | 13 |
| 6. Zero-copy | 14 |
| 7. QoS is a contract | 12 |
| 8. Declaring and watching parameters | 06, 07 |
| 9. Composition | 15 |

If you check out the source repository of `ros2/rclcpp` (`https://github.com/ros2/rclcpp`) at the Jazzy tag, you can also read the `.cpp` side of the headers quoted here (such as `node_topics.cpp` and `intra_process_manager.cpp`). This document stays within what can be read from the documentation comments and declarations of the headers, and does not go into the implementation bodies (`.cpp`). If you want to follow the behavior more accurately, please refer to those as well.
