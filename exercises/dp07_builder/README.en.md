# dp07 Builder [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 7. **There are two things called "Builder"**, so you implement both.
Only the name is the same. They solve different problems. Do not mix them.

| | Builder in Yuki's book | Builder in practice |
| --- | --- | --- |
| Problem it solves | make different representations from the same steps | C++ has no named arguments |
| Actors | Director / Builder / ConcreteBuilder | only one Builder |
| File | `src/telemetry_builder.cpp` | `src/motor_config.cpp` |

## What to do

### A. The form in Yuki's book — `src/telemetry_builder.cpp`

1. **`TelemetryDirector::construct()`**
   - Call `builder_` 5 times in the fixed order
   - The Director's job is to **write no commas and no braces at all**.
     Only the ConcreteBuilder knows the format

2. **`CsvTelemetryBuilder`**
   ```
   # robot telemetry
   battery_voltage,12.5
   motor_current,3.25
   cpu_temperature,41
   # end
   ```

3. **`JsonTelemetryBuilder`**
   ```
   {
     "title": "robot telemetry",
     "battery_voltage": 12.5,
     "motor_current": 3.25,
     "cpu_temperature": 41
   }
   ```
   - Put the comma **before the item**. If you put it after, an extra comma remains on the last item

### B. The form in practice — `src/motor_config.cpp`

4. **The 7 setters of `MotorConfigBuilder`**
   - The return type is `MotorConfigBuilder &`. **If you return by value, the whole Builder is copied at every step of the chain**
   - `name()` takes its argument by value, so move it with `std::move`

5. **`MotorConfigBuilder::build() const &`**
   - If `motor_id` is not set, return `std::nullopt`
   - Otherwise **copy** `config_` and return it (the Builder may still be used after this)

6. **`MotorConfigBuilder::build() &&`**
   - After the same check, **`std::move`** `config_` and return it
   - This is called only on a temporary object, so it is fine to take the contents away

`ControlLimitsBuilder` (in the lower part of the header) is a **finished example**. It is not an exercise.
It is an example where, if you build with only `constexpr`, it can be placed in ROM without running a single instruction at run time. Please read it.

## Run it

```bash
./drill run dp07
```

## Common pitfalls

- If a setter returns `MotorConfigBuilder` (by value), the test
  "チェーンは同じBuilderの参照を返す" (the chain returns a reference to the same Builder) fails it by comparing addresses
- Implement **both** `build() &&` and `build() const &`. If you implement only one,
  the other way of calling it becomes a compile error
  (`error: 'this' argument to member function 'build' is an lvalue, but function has rvalue ref-qualifier`)
- If you `std::move` in `build() const &`, the Builder breaks and the second `build()` is empty.
  **The lvalue version is a copy**
- If you start writing the format in the Director, your hand slipped. The Director only calls `make_header` / `make_field` /
  `make_footer` in order
- `TelemetryDirector` holds `TelemetryBuilder &` by reference.
  **It cannot outlive the Builder.** This is a constraint that Java does not have

## Tests

```bash
./drill run dp07
```

There are 9 tests. They check not only the resulting string, but also
**the call order of the Director**, **that the chain causes no copies**,
and **that the `&&` version moves**.
The `constexpr` Builder is checked at compile time with `static_assert`.

## References

- [7. Builder](../../docs-en/patterns/07_Builder.md)
- [cppreference: std::optional](https://en.cppreference.com/w/cpp/utility/optional)
- [cppreference: ref-qualifier (reference qualifiers of member functions)](https://en.cppreference.com/w/cpp/language/member_functions)
- [cppreference: constexpr](https://en.cppreference.com/w/cpp/language/constexpr)
