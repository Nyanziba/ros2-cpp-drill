# Exercise 13: Executors and callback groups [Advanced]

You experience the problem covered by the official documents
[Using callback groups](https://docs.ros.org/en/jazzy/How-To-Guides/Using-callback-groups.html) and
[Executors (concepts)](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Executors.html)
as code that actually deadlocks.

The theme of this exercise is "using rclcpp effectively". In all the earlier exercises you could leave everything to
`rclcpp::spin()`, but in real work there are always situations where that is not enough. The typical example is the code covered in this exercise:
**waiting synchronously for a service response inside a subscriber callback**.

## What to do

Fill in the TODOs in `src/relay_with_service.cpp`.

| Item | Value |
| --- | --- |
| Node name | `relay_with_service` |
| Input topic | `trigger` (`std_msgs::msg::Int32`, depth 10) |
| Output topic | `sum` (`std_msgs::msg::Int32`, depth 10) |
| Service it depends on | `add_two_ints` (`example_interfaces::srv::AddTwoInts`) |
| Behavior | When it receives a value `v` on `trigger`, it sends `a = v, b = v` to `add_two_ints`, **waits for that response inside the callback**, and publishes the resulting `sum` to `sum` |

The class declaration (`include/drill/relay_with_service.hpp`) is provided. The required design has
the following four points.

1. Create two callback groups (`subscription_group_` and `client_group_`.
   Both are `rclcpp::CallbackGroupType::MutuallyExclusive`).
2. Create the `"trigger"` Subscription by setting `subscription_group_` in the `callback_group` of
   `rclcpp::SubscriptionOptions`.
3. Create the `"add_two_ints"` client by passing `client_group_` as the third argument
   (the callback group) of `create_client`.
4. Inside the callback, wait for the response with `future.wait_for(...)`
   (do **not** use `rclcpp::spin_until_future_complete()`. The reason is below).

## Why does it deadlock

An Executor **runs only one callback at a time** from one `MutuallyExclusive` callback group.
This is rclcpp's default safety measure to prevent "several callbacks touching and breaking
the state of the same node at the same time".

Now suppose you leave the subscriber and the service client in the same callback group
and write code like this.

```cpp
void trigger_callback(const std_msgs::msg::Int32 & msg)
{
  auto future = client_->async_send_request(request);
  future.wait_for(2s);   // block here and wait for the response
  ...
}
```

When the response of `add_two_ints` comes back, rclcpp internally queues a callback to run
the "client completion handling". But this callback belongs to the same callback group as `client_`.
And `trigger_callback` itself also belongs to the same group. The Executor is now running `trigger_callback`,
and only one callback can run at a time from that group, so
the "client completion handling" is not run until `trigger_callback` finishes.

But `trigger_callback` is waiting for that "client completion handling" to run
and fill in the contents of the future.

- `trigger_callback` is waiting for the completion handling to finish
- The completion handling is waiting for `trigger_callback` to finish (because they are in the same group)

This becomes a cycle, and neither can move until the timeout passed to `future.wait_for()` runs out, or
forever. This is the deadlock you experience in this exercise.
It is not solved by `SingleThreadedExecutor`, of course, and also not by increasing the number of threads in a `MultiThreadedExecutor`
unless you split the callback groups. This is because you have not given the Executor the information that "callbacks in different groups
may run at the same time".

The solution is to put the subscriber and the client in **different** callback groups,
and to give the Executor enough capacity (threads) to process several callbacks at the same time.
Only when both are in place can another thread run the "client completion handling"
while `trigger_callback` is waiting.

## MutuallyExclusive and Reentrant

There are two kinds of callback groups.

- **MutuallyExclusive** (the default): Only one callback belonging to the group runs at a time.
  It gives you the comfort that no data race occurs even when several callbacks touch the same member variable.
  On the other hand, in a setup where "waiting inside the group" happens, as in this exercise,
  it causes a deadlock.
- **Reentrant**: Several callbacks belonging to the group may run on several threads
  **at the same time**. Even the same callback may run in parallel with itself.
  Throughput goes up, but the responsibility for making it thread-safe, such as locking access to member variables yourself,
  moves to the writer.

In this exercise, both groups stay `MutuallyExclusive`.
What you need to "resolve the deadlock" is not to make them Reentrant but
to **split the groups**. Since the subscriber and the client are originally
unrelated work, it is enough if each is exclusive within its own group.

## SingleThreadedExecutor / MultiThreadedExecutor / StaticSingleThreadedExecutor

- **SingleThreadedExecutor**: Processes the callbacks that become ready, in order, on one thread.
  All the earlier exercises (01 to 07) were fine with this.
  No matter how many callback groups you split, if there is only one thread to run them,
  they never run "at the same time", so the setup of this exercise always deadlocks.
- **MultiThreadedExecutor**: Processes the callbacks that become ready in parallel on several threads.
  If the callback groups are split, callbacks in different groups
  can run at the same time on different threads. This is what you use in this exercise.
- **StaticSingleThreadedExecutor**: Like SingleThreadedExecutor, it is one
  thread, but it builds the list of entities it waits on (Subscriptions, Services, and so on) statically only once
  when spin starts. For a setup where entities do not increase or decrease at run time,
  it has less overhead than SingleThreadedExecutor. It is still one thread,
  so it is not a way to fix the setup of this exercise.

## Run it

After the tests pass, you can actually run it by hand and check. First start the server
in another terminal (if you do not have the `server` of `drill_04_service_server`,
the `add_two_ints_server` of `demo_nodes_cpp` is fine too).

```bash
source install/setup.bash
ros2 run drill_04_service_server server
# if you do not have it: ros2 run demo_nodes_cpp add_two_ints_server
```

In another terminal, start `relay_with_service`.

```bash
source install/setup.bash
ros2 run drill_13_executors relay_with_service
```

In yet another terminal, subscribe to `sum`,

```bash
ros2 topic echo /sum
```

and from one more terminal, publish to `trigger`.

```bash
ros2 topic pub /trigger std_msgs/msg/Int32 "{data: 21}" --once
```

If `data: 42` appears on the `ros2 topic echo /sum` screen, it works. As an experiment, if you change the
`MultiThreadedExecutor` in `src/relay_with_service_main.cpp` to
`SingleThreadedExecutor` and run it (a temporary change for checking is fine.
Always change it back when you are done), you can confirm that nothing is
published to `sum`.

## Common pitfalls

- If you reuse one variable for the return value of `create_callback_group()`,
  `subscription_group()` and `client_group()` return the same object,
  and the deadlock happens. Call it **twice** and put the results into separate members.
- Even if you create `SubscriptionOptions`, if you forget to assign `callback_group`,
  the subscriber stays in the default group (= the node's `default_callback_group`).
- The third argument of `create_client` is the callback group. You cannot skip the first argument (the service name)
  and the second argument (the QoS) and pass the group directly, so
  you need to put `rclcpp::ServicesQoS()` in explicitly.
- You may be tempted to write something like `rclcpp::spin_until_future_complete(shared_from_this(), future)`
  inside the callback, but you cannot use it. From inside a callback that is already running
  in an Executor, it tries to start spinning again on the same node,
  which causes an exception or a deadlock. Wait with `future.wait_for()`.

## Tests

```bash
./drill run 13
```

| Test | What it checks |
| --- | --- |
| `サブスクライバとクライアントが別のコールバックグループにいる` (the subscriber and the client are in different callback groups) | Whether the two groups are created separately |
| `triggerに21を送るとsumに42がpublishされる` (sending 21 to trigger publishes 42 to sum) | The whole behavior, and that it does not deadlock |
| `連続してtriggerを送っても毎回応答する` (it responds every time even when trigger is sent repeatedly) | Whether it is solved as a structure and not by a one-time coincidence |
| `負の値でも正しく計算する` (calculates correctly even with negative values) | Whether it does not break at boundary values |

## References

- Official: [Using callback groups](https://docs.ros.org/en/jazzy/How-To-Guides/Using-callback-groups.html)
- Official: [About Executors](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Executors.html)
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
