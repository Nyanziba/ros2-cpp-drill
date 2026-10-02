# cppb02 Scope and lifetime [C++ Basics]

Understand the lifetime and scope of objects.

## What to do

Implement the Tracer class. The constructor and destructor append to a global log.

## Run it

```bash
./drill run cppb02
```

## Common pitfalls

- When execution leaves a scope, objects are destroyed in the reverse order of creation.
- RAII pattern: initialize when you acquire a resource, release it when the object is destroyed.

## Tests

```bash
./drill run cppb02
```

| Test | What it checks |
| --- | --- |
| `スコープを抜けるとき逆順に破棄される` (objects are destroyed in reverse order when leaving the scope) | reverse-order destruction |

## References

- [2. Scope and lifetime](../../docs-en/cpp-basics/02_scope_and_lifetime.md)
