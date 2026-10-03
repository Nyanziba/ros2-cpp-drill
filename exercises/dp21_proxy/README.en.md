# dp21 Proxy [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 21. You write the lazy creation of `PrinterProxy` in C++.
In C++, the core of Proxy is **writing `operator->`**, not inheritance.

## What to do

Implement 5 things in `src/calibration_proxy.cpp`.
The real-object side (`CalibrationTable` / `RegisterFile`) is already implemented.

1. **`CalibrationProxy::operator->()`**
   - If `real_` is empty, create it with `std::make_unique<CalibrationTable>(source_)` and return `real_.get()`
   - It is a `const` member function. `real_` is `mutable`, so you can rewrite it inside

2. **`RegisterAccess::RegisterAccess()`** — `owner_.record("enter");`

3. **`RegisterAccess::~RegisterAccess()`** — `owner_.record("leave");`

4. **`RegisterAccess::operator->()`** — `return &owner_.file_;`
   - A pointer is returned here, so the chain of `operator->` stops

5. **`SafeRegisterProxy::read()` / `write()`** — range check and recording

### The format of the record (log)

The test checks the strings for an exact match.

| Situation | String to record |
| --- | --- |
| Creating a `RegisterAccess` | `enter` |
| Destroying a `RegisterAccess` | `leave` |
| A read in range | `read:1` |
| A write in range | `write:1=255` |
| A read out of range | `reject:read:99` |
| A write out of range | `reject:write:4` |

## Run it

```bash
./drill run dp21
```

## Common pitfalls

- `error: no viable overloaded '='` / `but method is not marked const`
  → `operator->` is `const`. Look at `mutable std::unique_ptr<...> real_;` in the header
- `RealObjectIsNotCreatedUntilFirstAccess` fails
  → You are creating the real object in the constructor of `CalibrationProxy`
- `RealObjectIsNotRecreatedOnLaterAccess` fails
  → You forgot the `if (!real_)` check and call `make_unique` every time
- `proxy->write_raw(...)` does not work
  → `RegisterAccess::operator->()` returns `nullptr`.
    It is a `friend` of `SafeRegisterProxy`, so it can reach `owner_.file_` directly
- It fails with "弾いたのに本体に触っています" (it touched the real object even though it rejected)
  → You call `file_.read_raw()` / `file_.write_raw()` in the out-of-range branch.
    On the real robot, this destroys another peripheral
- The numbers of `enter` / `leave` do not match
  → You use `operator->` inside `read()` / `write()`.
    The checked path does not go through a temporary object

## Tests

```bash
./drill run dp21
```

There are 9 tests. They check how many times the lazy creation happens, lazy creation from a `const` Proxy,
that out-of-range accesses do not reach the real object, the chain of `operator->`, and the lifetime of the temporary object.

## References

- [21. Proxy](../../docs-en/patterns/21_Proxy.md)
- [cppreference: operator->](https://en.cppreference.com/w/cpp/language/operator_member_access)
- [cppreference: mutable](https://en.cppreference.com/w/cpp/language/cv)
- [cppreference: copy elision](https://en.cppreference.com/w/cpp/language/copy_elision)
