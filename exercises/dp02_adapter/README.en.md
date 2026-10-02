# dp02 Adapter [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 2, Adapter. You write **both the delegation version and the inheritance version**, and compare them.

## Subject

You adapt `LegacyMotorDriver` (a raw driver) written by a senior colleague three years ago
to the team's common interface `MotorActuator` (units are rad/s and rad).

| Role | Class |
| --- | --- |
| Target (the shape you want to match) | `MotorActuator` |
| Adaptee (the existing thing you cannot change) | `LegacyMotorDriver` |
| Adapter | `DelegatingMotorAdapter` / `InheritingMotorAdapter` |

You **cannot rewrite** `LegacyMotorDriver`. It has no virtual functions and no virtual destructor.
The setting is that 5 other projects depend on it.

## What to do

Implement the 6 functions in `src/motor_adapter.cpp`.

1. **`DelegatingMotorAdapter` (delegation version)** — it **holds the raw driver as a member**
   - `set_velocity()` / `stop()` / `position_rad()`
2. **`InheritingMotorAdapter` (inheritance version)** — it inherits the raw driver **privately**
   - The same 3 functions. The behavior must match the delegation version exactly

The conversion rules are as follows.

```
pulse command = std::lround(rad_per_sec * PULSES_PER_RAD_PER_SEC)   // 100 pulse per rad/s
angle [rad] = readEncoderRaw() / COUNTS_PER_RAD                    // 200 count per rad
```

Do not edit `include/drill/*.hpp` and `test/test_exercise.cpp`.

## Run it

```bash
./drill run dp02
```

## Common pitfalls

- The inheritance version uses private inheritance, so call **`setPulse(...)` directly**, not `driver_.setPulse(...)`
- If you remove the virtual destructor of `MotorActuator`, deleting through `unique_ptr<MotorActuator>` is undefined behavior
- A pulse is an `int`. Round it with `std::lround`, not by the truncation of `static_cast<int>`
- 50.0 rad/s is equivalent to 5000 pulses, but the driver side rounds it to 1000. **Do not round on the Adapter side**

## Things to think about (the tests do not check them, but they are the main point of the article)

- What breaks if you change `private` to `public` in the inheritance version
- What happens if `LegacyMotorDriver` also has virtual functions and you want to inherit from both
- Which version of Adapter is `std::stack` over `std::deque`

## References

- [2. Adapter](../../docs-en/patterns/02_Adapter.md)
- [Inheritance](../../docs-en/cpp/03_inheritance.md)
- [Smart pointers](../../docs-en/cpp/06_smart_pointers.md)
