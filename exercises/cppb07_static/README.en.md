# cppb07 The three meanings of static [C++ Basics]

Learn the three contexts of static.

## What to do

Implement the following three things in `src/counter.cpp`.

1. static inside a function: a variable that is initialized only the first time
2. static class member: a variable shared among all instances (the definition at file scope is required)
3. static class member function: a function that has no this

## Run it

```bash
./drill run cppb07
```

## Common pitfalls

- A static inside a function is initialized only the first time and stays alive after that.
- A static class member must always be defined in the `.cpp` file.
- A static inside a function and a static class member are different things.

## Tests

```bash
./drill run cppb07
```

| Test | What it checks |
| --- | --- |
| `関数内Staticは値を保持` (static inside a function keeps its value) | static inside a function |
| `クラスStaticメンバは共有される` (static class members are shared) | static class member and its definition |

## References

- [7. static](../../docs-en/cpp-basics/07_static.md)
