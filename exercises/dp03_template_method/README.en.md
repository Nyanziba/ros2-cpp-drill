# dp03 Template Method [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 3. You write Template Method with C++ **NVI (Non-Virtual Interface)**.

The steps of reading a sensor are fixed.

```
initialize (first time only) → get raw value → convert to physical quantity → validate
```

The base class `SensorReader` holds **only this order**, and derived classes fill in the content of each step.

## What to do

Implement `src/sensor_reader.cpp`. Do not edit `include/drill/sensor_reader.hpp`.

1. **`SensorReader::read_once()`** — the template method (the skeleton of the steps)
   - Call the 4 steps above in this order
   - Just before calling each step, call `record("step name")`
   - Initialize only the first time
   - If validation fails, return `std::nullopt`

2. **`SensorReader::validate()`** — the default validation (whether the value is finite)

3. **`EncoderReader`** — count value → angle [deg]

4. **`ThermistorReader`** — AD value → temperature [degC], reject values out of range
   - `validate()` calls `SensorReader::validate()` first, and then checks the range

## Run it

```bash
./drill run dp03
```

## Common pitfalls

- **Do not put `virtual` on `read_once()`.** The skeleton must not be replaceable. It corresponds to a `final` method in Java
- The virtual function of each step is `private`. **You can override a private function** (you just cannot call it)
- Only `validate()`, which a derived class wants to call the base version of, is `protected`
- Always write `override`. If you drop one `const`, it silently becomes a different function
- **Do not call `read_once()` from a constructor** (the derived implementation does not work yet)

## Tests

They look at `call_log()` and check **whether the order of the steps is kept**.
Even if the values are right, the test fails if the order is wrong.

## References

- [3. Template Method](../../docs-en/patterns/03_TemplateMethod.md)
- [C++ 3. Inheritance](../../docs-en/cpp/03_inheritance.md)
