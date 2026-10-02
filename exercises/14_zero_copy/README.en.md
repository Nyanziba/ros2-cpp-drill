# Exercise 14: Zero copy (unique_ptr and intra-process communication) [Advanced]

You check with your own hands the point from the official document
[Intra-Process Communication (demo explanation)](https://docs.ros.org/en/jazzy/Tutorials/Demos/Intra-Process-Communication.html):
"`publish(value)` and `publish(std::move(unique_ptr))` take different paths for the data that is actually carried".

## What to do

Fill in the TODOs in `src/zero_copy_nodes.cpp`.

| Item | Value |
| --- | --- |
| Talker node name | `zero_copy_talker` |
| Listener node name | `zero_copy_listener` |
| Topic name | `zero_copy` |
| Type | `std_msgs::msg::String` |
| QoS depth | 10 |
| `data` of the Talker | The address of the message itself, as a hexadecimal string |

The class declaration (`include/drill/zero_copy_nodes.hpp`) is provided. Both classes
take `const rclcpp::NodeOptions & options` in the constructor and
pass it as it is to `Node("...", options)`. This is because it is the caller (the tests or `zero_copy_demo_main.cpp`) that
decides whether to enable intra-process communication.

## The difference between `publish(value)` and `publish(std::move(unique_ptr))`

```cpp
auto message = std_msgs::msg::String();
message.data = "...";
publisher_->publish(message);              // (1) pass by value
```

```cpp
auto msg = std::make_unique<std_msgs::msg::String>();
msg->data = "...";
publisher_->publish(std::move(msg));       // (2) hand over ownership
```

(1) passes a "value", so rclcpp first copies the message once at the entrance of publish
and then puts it in the intermediate buffer (IntraProcessManager). The caller's
`message` stays alive as that local variable, so there is no way to avoid
the copy.

(2) hands over the "ownership" of the `unique_ptr` as it is with `std::move`, so no copy
happens at all. The caller no longer has `msg` (it was `move`d), so
rclcpp stores that memory as it is in the intermediate buffer, and if the conditions are met, it can deliver it
to the subscriber with the same address.

## When intra-process communication works and when it does not

Conditions where it works (all of them must be met):

- The Publisher and the Subscription are in the **same process**.
- Both nodes are created with `rclcpp::NodeOptions().use_intra_process_comms(true)`.
  **The default is `false`**, so it does not work unless you enable it explicitly
  (`rclcpp/node_options.hpp`: `bool use_intra_process_comms_ {false};`).
- The publishing side hands over ownership with `std::move(unique_ptr)`.
- The History of the QoS is `KeepLast` / `KeepAll` (a form that has a depth).

Conditions where it does not work (it goes back to copying):

- The Publisher and the Subscription are in different processes (separate `ros2 run` / separate node processes).
  The communication goes over DDS, so serialization and deserialization are needed.
- **`use_intra_process_comms` is not enabled.** Just putting them in the same process
  does not work (in our local measurement too, with the same process and the default options, the address of the
  message did not match between the sender and the receiver).
- A "value" is passed to publish (`publish(message)`).
- There are several subscribers, and some of them want to receive it in a **modifiable form** (`UniquePtr` / `SharedPtr`).
  A copy is made for them to satisfy sole ownership.

### About the type of the subscriber argument

The forms accepted by `rclcpp/any_subscription_callback.hpp` are
`const T &` / `T::UniquePtr` / `T::ConstSharedPtr` / `const T::ConstSharedPtr &` /
`T::SharedPtr` (plus the versions with `MessageInfo` and the serialized versions). **The "receive by value" form
is not supported in the first place** (it does not compile).

If intra-process communication is enabled, receiving as `const T &` does not cause a copy either
(measured on Jazzy. The addresses of the sender and the receiver match). Choose the type not by "whether there is a copy" but by
**how you want to handle the message**.

| Way of receiving | Meaning |
| --- | --- |
| `const T &` | Only read it on the spot. You cannot touch it after leaving the callback |
| `T::ConstSharedPtr` | Read only. You can keep it or pass it on |
| `T::UniquePtr` | Sole ownership. You can modify it and publish it again |

Also, intra-process communication itself worked with a `transient_local` QoS too (measured on Jazzy).
But "resending to a subscriber that joined later" is outside the intra-process buffer,
so think about that separately.

## Why this works for large data

Images (`sensor_msgs::msg::Image`) and point clouds (`sensor_msgs::msg::PointCloud2`) can be
several hundred KB to several MB. `publish(value)` copies this whole data in memory
before carrying it, so the number of copies grows linearly as the number of stages in the pipeline increases.
With `publish(std::move(unique_ptr))` the number of copies is 0, so even in the same pipeline
you save a lot of CPU time and memory bandwidth.
The `CameraNode` in `/opt/ros/jazzy/include/intra_process_demo/image_pipeline/camera_node.hpp`
publishes camera images with exactly this technique.

## Run it

After the tests pass, you can check in the log that the addresses match, using the `zero_copy_demo` executable.

```bash
source install/setup.bash
ros2 run drill_14_zero_copy zero_copy_demo
```

`Published message with address: 0x...` and `Received message with address: 0x...`
appear alternately, and within the same run they should always be the same address.

For comparison, it is also good to run `two_node_pipeline` in the official `intra_process_demo` package
(a simplified `camera_node`, an example with only the two nodes, sender and receiver).

```bash
ros2 run intra_process_demo two_node_pipeline
```

## Common pitfalls

- Always assign the return values of `create_publisher()` / `create_subscription()` to member variables.
- If you pass the `msg` created in `publish_once()` by value, as in `publisher_->publish(msg)`,
  a copy happens at that point and test 2 (addresses match) fails.
  Do not forget `std::move(msg)`.
- Keep the callback argument as the already declared `ConstSharedPtr`.
  (Zero copy itself also works with `const &`, but if it does not match the declaration in the header and the signature,
  the build does not pass. See the section above on how to choose the type.)
- Pass the `NodeOptions` you received in the constructor as it is to `Node("...", options)`.
  If you write `Node("...")` here, `use_intra_process_comms` is not passed on,
  a copy always happens, and the tests fail.

## Tests

```bash
./drill run 14
```

| Test | What it checks |
| --- | --- |
| `zero_copyトピックで通信できている` (communicates on the zero_copy topic) | Whether the Publisher / Subscription are running |
| `送信側と受信側のアドレスが一致する` (the addresses of the sender and the receiver match) | Whether it publishes with `std::move`, and whether the subscriber uses `ConstSharedPtr` |
| `dataの16進文字列も同じアドレスを指している` (the hexadecimal string in data also points to the same address) | Whether the address is written correctly into `data` |
| `複数回publishしても毎回アドレスが一致する` (the addresses match every time even if it publishes many times) | Whether it is properly zero copy every time |

## References

- Official: [Intra-Process Communication (demo explanation)](https://docs.ros.org/en/jazzy/Tutorials/Demos/Intra-Process-Communication.html)
- Official demo: [ros2/demos intra_process_demo](https://github.com/ros2/demos/tree/jazzy/intra_process_demo)
- Local example implementation: `/opt/ros/jazzy/include/intra_process_demo/image_pipeline/camera_node.hpp`
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
