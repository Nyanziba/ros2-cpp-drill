# 13. Minimal concurrency

> **Goal of this chapter**: In the drill, `future` appears **50 times** and `mutex` appears **9 times**.
> Beginners practice "creating threads by themselves", but ROS 2 is different.
> The rclcpp `Executor` owns the threads, and your code **runs as callbacks** inside it.
> So "creating a thread" never appears in the drill.
> But if you do not understand what happens inside the Executor,
> you will be caught by a "deadlock", where one callback blocks the execution of another callback.
> This chapter is not "the whole picture of concurrency". It is the minimum you need to avoid the pitfalls you actually step on in the drill.

## 13.1 `std::thread` — creating and ending threads

```cpp
#include <iostream>
#include <thread>

void worker(int id)
{
  std::cout << "Thread " << id << " is running\n";
}

int main()
{
  std::thread t1(worker, 1);
  std::thread t2(worker, 2);

  t1.join();   // wait for t1 to finish
  t2.join();   // wait for t2 to finish

  std::cout << "Both threads finished\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/djrhvK59r)

```
Thread 1 is running
Thread 2 is running
Both threads finished
```

You pass a function (or a function object, or a lambda) and its arguments to the constructor of `std::thread`.
The thread starts running immediately.

It is fine if the main thread ends first. But if the destructor of a `std::thread` object is called
when neither `join()` nor `detach()` has been done, **it calls `std::terminate` and forcibly stops the program.**

```cpp
#include <iostream>
#include <thread>

void worker() { std::cout << "Thread running\n"; }

int main()
{
  std::thread t(worker);
  // leave main without join or detach
  return 0;  // ← the destructor of t calls std::terminate
}
```

```
terminate called without an active exception
```

### `join()` and `detach()`

| Method | Behavior | Use |
| --- | --- | --- |
| `.join()` | Waits for completion. Synchronizes the thread's return value and exception | Almost always this in the drill |
| `.detach()` | Makes it independent. The thread continues even if main ends first | Daemon threads |

After you call either of them, `.joinable()` becomes `false`.
**Be careful: calling one twice throws an exception.**

## 13.2 Data race — writing a shared variable without protection

First, one warning. **A data race does not always show up when you write one.**
This is the most troublesome property of this bug, so let us first look at an example where it does not show up.

```cpp
#include <iostream>
#include <thread>
#include <vector>

int counter = 0;

void increment()
{
  for (int i = 0; i < 100000; ++i) {
    ++counter;                 // writing a shared variable without protection
  }
}

int main()
{
  std::vector<std::thread> threads;
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back(increment);
  }
  for (auto & t : threads) {
    t.join();
  }
  std::cout << "Result:   " << counter << "\n";
  std::cout << "Expected: 500000\n";
  return 0;
}
```

```bash
$ g++ -std=c++17 -Wall -Wextra -pthread race1.cpp -o race1
$ for i in 1 2 3 4 5 6 7 8; do ./race1 | head -1; done
結果:   500000
結果:   500000
結果:   500000
結果:   500000
結果:   500000
結果:   500000
結果:   500000
結果:   500000
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/EaaqGrcrE)

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/c54aMWoWr)

**All 8 runs gave the right answer.** The result was the same with `-O2`, and without `-pthread`
(measured on a 12-core machine).

This code is **broken**. It still gives the right answer because
one thread finishes its 100000 loops in less than 0.5 milliseconds, and
while `threads.emplace_back` creates the threads one by one,
**the earlier thread has already finished.** If there is no one to race with, there is no race.

Here is the most important fact in this chapter: **"it worked, so it is correct" does not hold.**

### Making them really race

Line the threads up, start them all at once, and increase the amount of work.

```cpp
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int counter = 0;
std::atomic<bool> go{false};

void increment()
{
  while (!go) { }                    // wait until all threads are ready
  for (int i = 0; i < 1000000; ++i) {
    ++counter;
  }
}

int main()
{
  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i) {
    threads.emplace_back(increment);
  }
  go = true;                          // start all at once
  for (auto & t : threads) {
    t.join();
  }
  std::cout << "Result:   " << counter << "\n";
  std::cout << "Expected: " << 8 * 1000000 << "\n";
  return 0;
}
```

```bash
$ g++ -std=c++17 -Wall -Wextra -pthread race2.cpp -o race2
$ for i in 1 2 3 4 5; do ./race2 | head -1; done
結果:   1352695
結果:   1408128
結果:   1427199
結果:   1436394
結果:   1467341
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/rTj9fbPcb)

**Against the expected 8000000, the result is around 1.4 million. More than 80% of the increments are lost.**
And the value is different every time.

`++counter` looks like one operation, but in machine code it is three steps:
`load` → `add` → `store`.
If two threads `load` the same value, and both add 1 to that same value and `store` it,
**something that should increase twice increases only once.** This happens a huge number of times.

### Detecting it — ThreadSanitizer

It is impossible to find a bug that "happens to work" by eye. **Use a tool.**

```bash
$ g++ -std=c++17 -fsanitize=thread -g -pthread race2.cpp -o race2_ts
$ ./race2_ts
==================
WARNING: ThreadSanitizer: data race (pid=134773)
  Write of size 4 at 0x591893f74154 by thread T6:
    #0 increment() race.cpp:13
  Previous read of size 4 at 0x591893f74154 by thread T7:
    ...
```

**`-fsanitize=thread` detects a race even when it does not actually show up.**
It also reports the first `race1.cpp` (the code that gave the right answer in all 8 runs).
Do not say "it is fine because the test passed". Make sure you can say **"it is fine because it passed TSan".**

It makes the program 5 to 15 times slower, so you do not put it in a production build. Running it in CI is the standard practice.

## 13.3 `std::mutex` and `std::lock_guard` — mutual exclusion

To prevent a data race, limit access to the shared variable to one thread at a time.

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

int counter = 0;
std::mutex counter_mutex;

void increment()
{
  for (int i = 0; i < 100000; ++i) {
    std::lock_guard<std::mutex> lock(counter_mutex);
    counter++;
  }
}

int main()
{
  std::vector<std::thread> threads;
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back(increment);
  }
  for (auto & t : threads) {
    t.join();
  }

  std::cout << "Result: " << counter << "\n";
  std::cout << "Expected: 500000\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/zf4ad3oaK)

```
Result: 500000
Expected: 500000
```

`std::lock_guard` is the RAII pattern from chapter 2.
The constructor locks the `mutex`, and the destructor unlocks it automatically.

It locks when it enters the RAII scope at `{`, and unlocks when it leaves at `}`.
The destructor is called even if an exception is thrown, so **it also prevents a deadlock (the program stopping while you still hold the lock).**

## 13.4 `std::atomic` — atomic operations with no lock

For a counter that is a single `int`, `atomic` is lighter than `mutex`.

```cpp
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

std::atomic<int> counter(0);

void increment()
{
  for (int i = 0; i < 100000; ++i) {
    counter++;  // atomic, so thread-safe
  }
}

int main()
{
  std::vector<std::thread> threads;
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back(increment);
  }
  for (auto & t : threads) {
    t.join();
  }

  std::cout << "Result: " << counter << "\n";
  std::cout << "Expected: 500000\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/vj6xPhhMG)

```
Result: 500000
Expected: 500000
```

`std::atomic<T>` wraps `T` and makes every operation **atomic** (indivisible).
`++` and `=` become a single CPU instruction instead of "read, then write", so no data race happens.

But its range of use is narrower than `mutex`. When you want to lock several variables at once, use `mutex`.

## 13.5 `std::promise` / `std::future` — a future value

To pass a value between threads, use `promise` and `future`.

```cpp
#include <iostream>
#include <thread>
#include <future>
#include <chrono>

int main()
{
  std::promise<int> prom;
  std::future<int> fut = prom.get_future();

  std::thread worker([&prom]() {
    std::cout << "Worker: computing...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    prom.set_value(42);
    std::cout << "Worker: done\n";
  });

  std::cout << "Main: waiting for the result\n";
  int result = fut.get();  // blocks
  std::cout << "Main: result = " << result << "\n";

  worker.join();
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/jdbG7dErn)

```
Main: waiting for the result
Worker: computing...
Worker: done
Main: result = 42
```

**A `future` is a promise that "the value is not here yet, but it will come later".**

- `promise` is "the side that makes the promise". It sets the value with `set_value()`
- `future` is "the side that waits for the promise". It receives the result with `get()` (blocks)

### Timeout with `wait_for`

```cpp
#include <iostream>
#include <thread>
#include <future>
#include <chrono>

int main()
{
  std::promise<int> prom;
  std::future<int> fut = prom.get_future();

  std::thread worker([&prom]() {
    std::cout << "Worker: waiting for a long time\n";
    std::this_thread::sleep_for(std::chrono::seconds(10));
  });

  std::cout << "Main: waiting 2 seconds\n";
  auto status = fut.wait_for(std::chrono::seconds(2));

  if (status == std::future_status::ready) {
    std::cout << "Done: " << fut.get() << "\n";
  } else if (status == std::future_status::timeout) {
    std::cout << "Timeout\n";
  } else if (status == std::future_status::deferred) {
    std::cout << "Deferred execution\n";
  }

  worker.join();
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/Mrx6dzE4q)

```
Main: waiting 2 seconds
Worker: waiting for a long time
Timeout
```

`wait_for` returns the state as a `std::future_status` enum.

| Return value | Meaning |
| --- | --- |
| `ready` | The value is available |
| `timeout` | Time ran out (the value has not arrived yet) |
| `deferred` | Not executed yet (when you use `std::launch::deferred` with async) |

In ROS 2 code, `wait_for` is used as a **timeout to avoid deadlock**.

## 13.6 Deadlock — the trap in the Executor

Now we come to the pitfalls you actually step on in the drill.

**In the drill, the rclcpp Executor manages the threads, and your callbacks run on top of them.**
You need to be careful when you call `future.wait_for()` from inside a callback.

### A real example: exercise 13

```cpp
// solutions/13_executors/src/relay_with_service.cpp (excerpt)

void RelayWithService::trigger_callback(const std_msgs::msg::Int32 & msg)
{
  auto request = std::make_shared<AddTwoInts::Request>();
  request->a = msg.data;
  request->b = msg.data;

  auto future = client_->async_send_request(request);

  if (future.wait_for(2s) != std::future_status::ready) {
    RCLCPP_ERROR(this->get_logger(), "add_two_ints response timed out");
    return;
  }

  std_msgs::msg::Int32 out;
  out.data = future.get()->sum;
  publisher_->publish(out);
}
```

This code itself is safe. The reason is that, as it is written,
**`subscription_group_` and `client_group_` are separate `MutuallyExclusive` groups,
and they run on a `MultiThreadedExecutor`.**

#### From the header file

```cpp
// drill/relay_with_service.hpp (excerpt)

RelayWithService::RelayWithService()
: Node("relay_with_service")
{
  subscription_group_ = this->create_callback_group(
    rclcpp::CallbackGroupType::MutuallyExclusive);
  client_group_ = this->create_callback_group(
    rclcpp::CallbackGroupType::MutuallyExclusive);

  // ...create the subscription in subscription_group_,
  // and the client in client_group_
}
```

### How the deadlock works

What if both belonged to the **same `MutuallyExclusive` callback group**?

```
Thread pool (1 thread)
  ↓
  subscription_group (MutuallyExclusive)
    ├─ trigger_callback is running
    │   └─ waiting in future.wait_for()...
    │       (waiting for the client's response)
    │
    └─ the client's completion handler cannot run
        (the group is locked)
```

`trigger_callback` is waiting for the completion callback, but that callback runs in the same group,
and the group does not run other callbacks "until `trigger_callback` finishes".

**It waits forever. That is a deadlock.**

### The fix: separate groups

The drill's answer avoids it by splitting into two `MutuallyExclusive` groups and
**using multiple threads** with a `MultiThreadedExecutor`.

```cpp
subscription_group_ = this->create_callback_group(
  rclcpp::CallbackGroupType::MutuallyExclusive);  // thread 1
client_group_ = this->create_callback_group(
  rclcpp::CallbackGroupType::MutuallyExclusive);  // thread 2
```

With two threads, while one is waiting, the other can run the client completion handler.

### A deadlock seen in plain C++

We reproduce the same pattern with promise/future.

```cpp
#include <iostream>
#include <thread>
#include <future>
#include <chrono>

// One worker thread cannot set_value to itself
std::promise<int> shared_prom;

void worker()
{
  std::cout << "Worker: I need to call set_value\n";
  std::cout << "Worker: but I am waiting in future.wait_for()...\n";
  // ← This is the problem. This thread is held by the future
  // so it cannot run set_value
}

int main()
{
  std::future<int> fut = shared_prom.get_future();

  std::thread w(worker);

  // I want to call set_value, but the worker thread does not have it
  // (the main thread has it)

  w.join();
  return 0;
}
```

In real work, the basic rule is: **"if you wait for a `future` inside a callback, make sure that callback is in a separate thread pool."**

For details, read chapter 5 "Callback groups" of [rclcpp design philosophy](../rclcpp_design_philosophy.md).

## Try it yourself

**See all the concepts of concurrency in one program.**

```cpp
// concurrency.cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <vector>
#include <chrono>

void example_join_detach()
{
  std::cout << "\n-- 1. join and detach\n";

  auto worker = []() { std::cout << "  Worker running\n"; };

  std::thread t1(worker);
  t1.join();
  std::cout << "  After join: joinable = " << t1.joinable() << "\n";

  std::thread t2(worker);
  t2.detach();
  std::cout << "  After detach: joinable = " << t2.joinable() << "\n";
}

void example_data_race()
{
  std::cout << "\n-- 2. Data race\n";

  int counter = 0;
  std::atomic<bool> go{false};
  auto inc = [&counter, &go]() {
    while (!go) { }                    // line up all threads before running
    for (int i = 0; i < 1000000; ++i) {
      ++counter;
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i) {
    threads.emplace_back(inc);
  }
  go = true;
  for (auto & t : threads) {
    t.join();
  }

  std::cout << "  Result: " << counter << " (expected: " << 8 * 1000000 << ")\n";
}

void example_mutex()
{
  std::cout << "\n-- 3. mutex\n";

  int counter = 0;
  std::mutex m;

  auto inc = [&counter, &m]() {
    for (int i = 0; i < 10000; ++i) {
      std::lock_guard<std::mutex> lock(m);
      counter++;
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < 3; ++i) {
    threads.emplace_back(inc);
  }
  for (auto & t : threads) {
    t.join();
  }

  std::cout << "  Result: " << counter << " (expected: 30000)\n";
}

void example_atomic()
{
  std::cout << "\n-- 4. atomic\n";

  std::atomic<int> counter(0);

  auto inc = [&counter]() {
    for (int i = 0; i < 10000; ++i) {
      counter++;
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < 3; ++i) {
    threads.emplace_back(inc);
  }
  for (auto & t : threads) {
    t.join();
  }

  std::cout << "  Result: " << counter << " (expected: 30000)\n";
}

void example_promise()
{
  std::cout << "\n-- 5. promise/future\n";

  std::promise<int> prom;
  std::future<int> fut = prom.get_future();

  std::thread w([&prom]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    prom.set_value(123);
  });

  int result = fut.get();
  std::cout << "  Result: " << result << "\n";

  w.join();
}

void example_wait_for()
{
  std::cout << "\n-- 6. wait_for\n";

  std::promise<int> prom;
  std::future<int> fut = prom.get_future();

  std::cout << "  Waiting 1 second...\n";
  auto status = fut.wait_for(std::chrono::seconds(1));

  if (status == std::future_status::timeout) {
    std::cout << "  Timeout\n";
  }
}

int main()
{
  std::cout << "=== Concurrency demo ===\n";

  example_join_detach();
  example_data_race();
  example_mutex();
  example_atomic();
  example_promise();
  example_wait_for();

  std::cout << "\nDone\n";
  return 0;
}
```

**Predict: What is printed in each example? In particular:**

1. Is `joinable()` after `join` 0 or 1?
2. Is the data race result 8000000, or smaller?
3. Are the results of mutex and atomic the same?

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -pthread concurrency.cpp -o concurrency && ./concurrency
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/a3cEMjd95)

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/3reoh1cnq)

**Run it 5 times in a row.** Check that only the data race line shows a different value each time,
and the mutex and atomic lines show the same value each time.

After that, make these 3 changes.

**① Stop lining up the start.**

Delete the line `while (!go) { }` and the line `go = true;`, and change the number of threads from 8 back to 3,
and the repeat count from 1000000 back to 10000.

```cpp
  auto inc = [&counter]() {
    for (int i = 0; i < 10000; ++i) {
      ++counter;
    }
  };
```

**This time the correct 30000 appears every time.** It is the same phenomenon you saw in section 13.2.
The code is still broken, but the result is correct.
**Why "it worked, so it is correct" does not hold** — confirm it with your own hands, twice.

**② Add `-fsanitize=thread`.**

Keep the state of ① where "the correct value appears every time", and run it through TSan.

```bash
g++ -std=c++17 -g -pthread -fsanitize=thread concurrency.cpp -o concurrency_ts && ./concurrency_ts
```

**Even if the result is correct, `WARNING: ThreadSanitizer: data race` appears.**
This is the difference between "passing a test" and "checking correctness".

(Also check that the mutex version and the atomic version are not reported.
TSan watches for "unprotected simultaneous access", so if it is protected correctly, it stays silent.)

**③ In the `wait_for` example, move the `promise` into the thread by value:**

```cpp
std::promise<int> prom;
std::future<int> fut = prom.get_future();

std::thread w([prom = std::move(prom)]() mutable {
  // ...
});
```

This moves the ownership of the promise to the thread. The main thread can no longer call set_value, so
the future waits forever. Check that the timeout happens for sure.

## Common pitfalls

**`error: std::thread::thread(const std::thread&)' is deleted`**

You are trying to copy a `std::thread`. A thread has only move semantics.
When you add one to a `std::vector<std::thread>`, use `emplace_back`.

**`error: call to 'lock_guard::lock_guard(const lock_guard&)' is deleted`**

You are copying a `lock_guard`. It is an RAII object, so it cannot be copied.
Use it only inside its scope.

**Abnormal exit: `terminate called`**

There are 3 common causes.

1. A `std::thread` was destroyed without `join` or `detach`
2. A `std::lock_guard` tried to lock twice (on the same mutex)
3. `future.get()` was called twice

All of them are "the order was wrong" or "a hole in the logic".
If you look at the stack trace with `gdb`, you can see where it stopped.

**`std::bad_weak_ptr` — `lock()` from a `weak_ptr` failed**

You tried to promote a `weak_ptr` that has no owner to a `shared_ptr`.
Check that the return value of `lock()` is not `nullptr`.

**The program suddenly stops (deadlock)**

Are you calling `future.wait()` / `future.get()` without a timeout?
Or are you calling `wait_for(long_time)` from inside a callback?
It is a circular dependency:
"wait on this thread for a callback to finish → the callback runs on the same thread".

Deadlocks are hard to detect, so the prevention is: **always add a timeout.**

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 04 | `async_send_request` returns a `std::shared_future`, and `.get()` is called without a timeout |
| 05 | `async_send_request` gives a future. `wait_for` with a timeout |
| 13 | Avoiding deadlock with two `MutuallyExclusive` callback groups |
| Tests | Waiting for the service response with `future.wait_for()` and a 100ms timeout |

Exercise 13 is the topic of this chapter itself.
If you start exercise 13 without reading this, "why we split into two callback groups on purpose" looks like a mystery.

## References

- [rclcpp design philosophy](../rclcpp_design_philosophy.md) chapter 5 — details of the Executor and callback groups
- `cppreference`: [std::thread](https://en.cppreference.com/w/cpp/thread/thread), [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex), [std::future](https://en.cppreference.com/w/cpp/thread/future)
- Anthony Williams, *C++ Concurrency in Action* — the standard textbook on concurrency (the 2nd edition covers C++17)

---

Previous → [12. chrono and time](12_chrono_and_time.md)
Next → [14. Error handling](14_error_handling.md)
