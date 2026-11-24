# bt01 Priority [Behavior trees]

One tick sets the blackboard trace to estop, avoid, score, or search.

## What to do

Implement `match_xml` and `register_match_nodes`. If estop, the trace is `estop` and the status is SUCCESS. Else if time_up, FAILURE and an empty trace (wrap TimeUp in an Inverter). Else if range_mm < 400, `avoid`. Else if see_marker, `score`. Otherwise `search`. The XML also contains a Fallback.

## Run it

```bash
./drill run bt01
```

## Common pitfalls

Running Trace on the time_up branch leaves text in a trace that must stay empty.

## Tests

```bash
./drill run bt01
```

| Test | What it checks |
| --- | --- |
| `XmlInvertsTimeUp` | Inverter and Fallback |
| `EstopOutranksEverything` | estop comes first |
| `TimeUpStopsTheRest` | time up leaves an empty trace |
| `AvoidBeatsScoreAndSearchIsLast` | avoid, score, then search |

## References

- [1. Priority](../../docs-en/behavior-tree/01_priority.md)
