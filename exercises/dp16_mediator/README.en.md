# dp16 Mediator [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 16. Using the robot's control panel as the subject, you build a structure where Colleagues (the parts) are not connected directly to each other,
and only the Mediator (`ControlPanel`) coordinates them.

The key point in C++ is **mutual reference**. The Mediator holds the Colleagues, and the Colleagues point to the Mediator.
If both are `std::shared_ptr`, **a circular reference means neither is freed**.

## Coordination rules

| State | Emergency stop toggle | Auto mode toggle | The 2 manual buttons |
| --- | --- | --- | --- |
| Both off | enabled | enabled | enabled |
| Auto mode on | enabled | enabled | **disabled** |
| Emergency stop on | enabled | **disabled** | **disabled** |

Only the emergency stop toggle is always enabled. If you disabled it, you could not release it.

## What to do

Implement 6 things in `src/control_panel.cpp`.

1. **`PanelWidget::set_mediator()`**
   - Just save it to `mediator_`. **Do not own it** (a raw pointer)

2. **`PanelWidget::notify_changed()`**
   - If `mediator_` is not `nullptr`, call `widget_changed(this)`
   - If it is `nullptr`, do nothing (because we may be in the middle of two-phase initialization)

3. **`ToggleWidget::set_checked()`**
   - If disabled, do nothing / if the value does not change, do nothing / if it changes, report

4. **`ButtonWidget::press()`**
   - If disabled, `false`. If enabled, increase the press count, report, and return `true`

5. **The constructor of `ControlPanel`**
   - Call `set_mediator(this)` on the 4 parts, and initialize with `update_enabled_states()`

6. **`ControlPanel::widget_changed()` and `update_enabled_states()`**
   - Record who it came from in `change_log_`, and decide enabled/disabled again according to the table above

## Run it

```bash
./drill run dp16
```

## Common pitfalls

- **Do not call `notify_changed()` from `set_enabled()`.**
  `widget_changed()` → `update_enabled_states()` → `set_enabled()` → `widget_changed()` is
  an infinite recursion. "The entry called from the Mediator" and "the entry called from a Colleague" are different things
- Do not touch another Colleague from inside a Colleague. The test
  "Mediatorを外すとColleague間に影響が伝わらない" (if you remove the Mediator, the effect is not passed between Colleagues) fails it
- Do not hold a `std::shared_ptr` in `set_mediator()`. The test
  "ColleagueはMediatorを所有しない" (a Colleague does not own the Mediator) checks it with `use_count()`
- You cannot wire them up in the constructor's initializer list. To make a Colleague you need the Mediator, and
  to pass the Mediator you need the Colleague. That is why it is two-phase initialization
- If you report when the value has not changed, the test "同じ値をもう一度入れても報告されない" (setting the same value again is not reported) fails

## Tests

```bash
./drill run dp16
```

There are 12 tests. In addition to the coordination rules, they check
**that it goes through the Mediator**, **that the Mediator owns the Colleagues**, and
**that destruction calls the destructors of all the Colleagues**.

## References

- [16. Mediator](../../docs-en/patterns/16_Mediator.md)
- [cppreference: std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
