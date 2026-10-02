# c03 Inherit and implement virtual functions [C++]

Learn inheritance and virtual functions (polymorphism).

## What to do

Implement two sensor classes in `src/sensor.cpp`:

1. **TemperatureSensor** — a temperature sensor
   - `read()` overrides and returns **25.0**
   - `label()` is not overridden (it uses the default "Sensor" of the base class)

2. **HumiditySensor** — a humidity sensor
   - `read()` overrides and returns **60.0**
   - `label()` overrides and returns **"HumiditySensor"**

## Run it

```bash
./drill run c03
```

## Common pitfalls

- A **pure virtual function** `= 0` must always be overridden in a derived class.
- A **virtual function** uses the `virtual` keyword, and a derived class uses the `override` keyword.
- The `override` keyword (C++11 and later) is useful because it detects a spelling mistake by accident.
- Check with the tests that polymorphism gets the right values.

## Tests

```bash
./drill run c03
```

| Test | What it checks |
| --- | --- |
| `TemperatureSensorが正しい値を返す` (TemperatureSensor returns the correct value) | the read() implementation of TemperatureSensor |
| `HumiditySensorが正しい値を返す` (HumiditySensor returns the correct value) | the read() implementation of HumiditySensor |
| `TemperatureSensorはデフォルトのlabelを使う` (TemperatureSensor uses the default label) | inheriting the default implementation |
| `HumiditySensorはlabelをoverrideしている` (HumiditySensor overrides label) | the override implementation |
| `ポリモーフィズムで正しくディスパッチされる` (dispatch is correct with polymorphism) | dynamic dispatch through a base class pointer |

## References

- [cppreference: Virtual function](https://en.cppreference.com/w/cpp/language/virtual)
- [cppreference: override specifier](https://en.cppreference.com/w/cpp/language/override)
- [3. Inherit and implement virtual functions](../../docs-en/cpp/03_inheritance.md)
