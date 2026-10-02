# dp06 Prototype [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 6. You copy a **template of a waveform pattern sent to a motor**.

Let me say this first. **C++ has a copy constructor.**
If `PulseTrain b = a;` can copy it, you do not need `clone()`.
You really need `clone()` **only when you hold nothing but a `std::unique_ptr<Waveform>`
and want to copy it without knowing the type of the actual object**
(section 6.1 of [6. Prototype](../../docs-en/patterns/06_Prototype.md)).

## What to do

Implement 5 places in `src/waveform.cpp`.

1. **`Waveform::clone()`**
   - Wrap the return value of `do_clone()` in a `std::unique_ptr<Waveform>` and return it
   - You cannot use `std::make_unique`, because you do not know the type of the actual object

2. **The copy constructor of `PulseTrain`**
   - Copy the contents of `other.pattern_` one by one (a deep copy)
   - The array allocation is already written. Your job is to **copy the contents**

3. **`PulseTrain::do_clone()`**
   - `new` a copy of itself and return it. Leave the contents to the copy constructor (one line)
   - The return type `PulseTrain *` is the **covariant return type**

4. **`SineSweep::do_clone()`**
   - The same. `SineSweep` has only value members, so you do not write a copy constructor (Rule of Zero)

5. **`WaveformLibrary::duplicate()`**
   - Call `clone()` on all elements and put them into a new library
   - Make sure you can copy **without knowing the type of any actual element**

Do not edit `include/drill/waveform.hpp` and `test/test_exercise.cpp`.

## Run it

```bash
./drill run dp06
```

## Common pitfalls

- You may want to write `std::unique_ptr<PulseTrain> do_clone()`, but it **does not compile**.
  `std::unique_ptr<Base>` → `std::unique_ptr<Derived>` cannot be a covariant return type.
  That is why `do_clone()` returns a raw pointer and `clone()` wraps it
- Do **not `override`** `clone()`. It is non-virtual.
  What you should override is the private `do_clone()`
- Do not forget `Waveform(other)` in the copy constructor of `PulseTrain`.
  If you forget it, the base part is default-constructed (you do not see it in this exercise,
  but it becomes a bug in a design where the base holds state)
- `pattern_` is a `std::unique_ptr<double[]>`, so **even if you try to write a shallow copy,
  it does not compile**. With a raw pointer member it compiles, and it crashes with a double free
- The copy of `WaveformLibrary` is `= delete`d
  because "if you hold a `std::vector<std::unique_ptr<...>>`, copying is automatically prohibited"
  is **a lie**. Read the comment in the header

## Tests

```bash
./drill run dp06
```

There are 8 tests.

| Test | What it checks |
| --- | --- |
| `cloneは元とは別のオブジェクトを返す` (clone returns an object different from the original) | whether the addresses differ |
| `unique_ptr経由でも派生の型が保たれる` (the derived type is kept even through unique_ptr) | the actual type, with `dynamic_cast` |
| `SineSweepもcloneで複製できる` (SineSweep can also be copied with clone) | whether it is the same for the other derived class |
| `cloneした波形は深いコピーになっている` (the cloned waveform is a deep copy) | whether the copy does not change when you change the original |
| `PulseTrainのコピーコンストラクタが深いコピーを作る` (the copy constructor of PulseTrain makes a deep copy) | a plain copy that does not go through `clone()` |
| `複製はバッファを共有しない` (the copy does not share the buffer) | comparing the addresses of `data()` |
| `duplicateは要素数と型を保つ` (duplicate keeps the number of elements and the types) / `duplicateした要素は元と共有されない` (the elements copied by duplicate are not shared with the original) | copying the whole library |

It also checks with `static_assert` that the classes whose copy should be prohibited (assignment to `Waveform`,
the copy of `WaveformLibrary`) are actually prohibited.

## References

- [6. Prototype](../../docs-en/patterns/06_Prototype.md)
- [C++ 5. Move and ownership](../../docs-en/cpp/05_move_and_ownership.md)
- [C++ 6. Smart pointers](../../docs-en/cpp/06_smart_pointers.md)
- [cppreference: Copy constructors](https://en.cppreference.com/w/cpp/language/copy_constructor)
