# dp08 Abstract Factory [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 8. You write the structure that **swaps a whole family of products at once**, in both a run-time version and a template version.

The subject is "a pair of motor output and encoder input".
There is a set of parts for the real robot and a set of parts for simulation, and **you must not mix these two**.
If you combine a real motor with a simulated encoder, the control runs away as it is.

## What to do

### `src/actuator_kit.cpp` (run-time version)

1. **4 concrete products** (inside an anonymous namespace)
   - `SimMotor` / `SimEncoder` / `HwMotor` / `HwEncoder`
   - Inside, they just hold the `*Core` of the header by value. `kit_id()` returns its own family

2. **`create_motor()` / `create_encoder()` / `kit_id()` of `SimulationKitFactory` / `HardwareKitFactory`**
   - Create with `std::make_unique` and return
   - **Pass the same bus / registers from the same factory.** This is the heart of Abstract Factory

3. **`run_open_loop()`**
   - You must be able to write it **without writing the name of any concrete factory**
   - If you cannot, the abstraction is not done

### `src/static_kit.cpp` (template version)

4. Write **`run_open_loop_static<KitTraits>()`**, and call it from
   `run_open_loop_static_sim` / `run_open_loop_static_hw`
   - For creation, you do not use `make_unique`; you just "place it right there"
   - No vtable and no heap allocation. On a microcontroller, this is the main choice

## Run it

```bash
./drill run dp08
```

## Common pitfalls

- If you pass **different buses** to `create_motor()` and `create_encoder()`, the family gets mixed.
  The test `PartsFromSameFactoryAreConnected` fails it
- If you write `SimulationKitFactory` inside `run_open_loop()`, the design is broken.
  The only argument is `const ActuatorKitFactory &`
- The real-robot side has a 4x encoder, so with the same duty the count advances 4 times as much.
  **A different family naturally behaves differently.** Do not try to match them
- In the template version, if you forget the `typename` of `typename KitTraits::Motor`, it is a compile error
- The virtual destructors of `MotorOutput` / `EncoderInput` / `ActuatorKitFactory` are on the header side.
  If you add a base class yourself, always write a virtual destructor

## Tests

There are 8 tests. They check that the same abstract code works on both families just by swapping the factory,
that the created parts belong to the same family, that ownership is with the caller,
and that the template version gives the same result as the run-time version.

## References

- [8. Abstract Factory](../../docs-en/patterns/08_AbstractFactory.md)
- [0. Before you use them](../../docs-en/patterns/00_before_you_use_them.md) — 0.3 "Creation becomes 3 steps"
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
