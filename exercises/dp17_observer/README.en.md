# dp17 Observer [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 17. This is **the peak of this course**.

The Java version of `addObserver()` returns `void` and that is the end of it. If you do that in C++,
**the moment a subscriber dies first, the Subject hits a dangling pointer.**
Here we solve it with the approach "return an RAII token that represents the subscription (`Subscription`)".

## What to do

Fill in TODO(1) to (8) in `src/sensor_hub.cpp`.

1. **`detail::Registry::remove()`**
   - Put `nullptr` into the `observer` of the `Entry` that matches the id (**only mark it**)
   - Call `compact()` only when `notifying` is false

2. **`detail::Registry::compact()`**
   - Actually remove the elements with `observer == nullptr`

3. **`Subscription::~Subscription()`**
   - Just call `reset()`. All the value of the token approach is in this one line

4. **`Subscription` (move constructor)**
   - Put the moved-from object back into the state "subscribing to nothing"

5. **`Subscription::operator=` (move assignment)**
   - Reject self-assignment → cancel the current subscription → take over → empty the other

6. **`Subscription::reset()`**
   - If `weak_ptr::lock()` is empty, **do nothing** (the Subject is already dead)
   - It must be safe to call twice

7. **`Subscription::active()`**
   - It must also be false when the Subject died first

8. **`SensorHub::subscribe()` / `publish()` / `observer_count()`**
   - `publish()` is in registration order. Watch 3 points: re-entry prevention, deferred removal, and an index loop

## Run it

```bash
./drill run dp17
```

## Common pitfalls

- **Do not call `entries.erase()` inside the notification loop.**
  The index of the running loop shifts, and some subscribers are skipped and some are called twice.
  Only mark it, and delete after the loop is finished
- **Do not keep a reference or an iterator.** If `subscribe()` is called during a notification,
  the `std::vector` is reallocated and what you kept becomes invalid. Loop with an index
- **Do not forget to reset the re-entry prevention flag.** To make it return even if `on_sample` throws an exception,
  it is safe to write a small RAII right there that sets it to false in the destructor
- **Empty the moved-from `Subscription`.** If you forget, the subscription is cancelled
  the moment the moved-from object dies (the same story as `unique_ptr`)
- **`registry_` is a `weak_ptr`.** If you hold it as a raw pointer, `~Subscription()` touches a dead object
  after the `SensorHub` died first

## Tests

```bash
./drill run dp17
```

There are 10 tests. Besides that the notifications arrive, they check

- that the Subject does not break even if an observer dies first
- that it does not crash even if you cancel during a notification
- that it does not loop forever even if the notifications form a cycle
- that `Subscription` is not copyable and is movable
- **that destroying the token is safe even if the Subject died first**

## References

- [17. Observer](../../docs-en/patterns/17_Observer.md)
- [cppreference: std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [cppreference: std::remove_if](https://en.cppreference.com/w/cpp/algorithm/remove)
