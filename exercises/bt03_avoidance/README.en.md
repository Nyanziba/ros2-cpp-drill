# bt03 Avoidance [Behavior trees]

Choose an AvoidCommand from the distances, and write the same result in a tree.

## What to do

`decide_avoidance` returns Stop when front < 150. Otherwise, when front < 400, it returns DodgeLeft if left >= right and DodgeRight otherwise. With no obstacle, Go if goal_visible and Search if not. The tree reads front_mm, left_mm, right_mm, and goal_visible, and sets command to stop, left, right, go, or search. Include a Fallback and a Sequence in the XML.

## Run it

```bash
./drill run bt03
```

## Common pitfalls

Fixing only the function and not the tree fails MatchesTheFunction.

## Tests

```bash
./drill run bt03
```

| Test | What it checks |
| --- | --- |
| `StopsBeforeDodging` | stop comes first |
| `MatchesTheFunction` | the tree matches the function |

## References

- [3. Avoidance](../../docs-en/behavior-tree/03_avoidance.md)
