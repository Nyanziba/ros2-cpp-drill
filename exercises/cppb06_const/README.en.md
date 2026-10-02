# cppb06 Match const in both declaration and definition [C++ Basics]

In this exercise you check the four positions of `const` by matching the header and the `.cpp` file.

## The header already has four consts

Do **not** edit `include/drill/config.hpp`. Read it first.

```cpp
class Config
{
public:
  Config(const int & limit);      // ① take the argument as read-only
  int get_limit() const;          // ② this member function does not change the object
  const int * ptr_to_limit() const;  // ③ what the returned pointer points to is read-only; same trailing const as ②
private:
  const int limit_;               // ④ the member itself cannot be changed
};
```

**The same `const` appears in four places, and each one means something different.** This is the theme of the chapter.

| Position | What becomes const |
| --- | --- |
| ① argument `const int &` | the caller's value is not changed |
| ② trailing `const` | the object that `this` points to is not changed |
| ③ return value `const int *` | what the returned pointer **points to** cannot be changed |
| ④ member `const int` | cannot be assigned after initialization (an initializer list becomes required) |

## What to do

You edit only `src/config.cpp`.
Right now the definitions have no trailing `const`, so **the declaration and the definition are treated as different functions.**

```
error: no declaration matches ‘int Config::get_limit()’
```

Make this error go away. You add `const` in **two places** (`get_limit` and `ptr_to_limit`).
① and ④ are about the header, so there is nothing to write for them here.

## Run it

```bash
./drill run cppb06
```

**While you have not started, the build does not pass.** The trailing `const` is part of the function type,
so forgetting it does not show up as "a test turns red". It shows up as "does not match the declaration".

## Common pitfalls

- The trailing `const` is **part of the function type**. That is why you need it in both the declaration and the definition.
  If you put it on only one side, you define a different function and get the error above.
- When `no declaration matches` appears, first compare the header declaration with your definition one character at a time.
- `const int * f()` and `int * f() const` are different. The former is about the return value, the latter is about `this`.

## Tests

| Test | What it checks |
| --- | --- |
| `ConstCorrect` | whether `get_limit()` can be called on a `const Config` (trailing const of ②) |
| `戻り値のポインタはconst` (the returned pointer is const) | whether the return type is `const int *` (③) |

## References

- [6. const](../../docs-en/cpp-basics/06_const.md)
