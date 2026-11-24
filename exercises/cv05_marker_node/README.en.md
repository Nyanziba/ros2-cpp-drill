# cv05 The marker detector node [Vision]

Subscribe to image and publish one line on marker.

## What to do

`format_detection` uses the same pixel rule as cv01. If elapsed_ms > budget_ms, return `stale`. Otherwise `red cx cy`, `blue cx cy`, or `none`. `MarkerDetector` uses topics image and marker, parameters min_pixels (default 3) and budget_ms (default 40), and KeepLast(10) reliable QoS.

## Run it

```bash
./drill run cv05
```

## Common pitfalls

SensorDataQoS can drop the one frame the test sends. Leave this exercise on reliable.

## Tests

```bash
./drill run cv05
```

| Test | What it checks |
| --- | --- |
| `FormatsRedBlueNoneAndStale` | the four strings |
| `PublishesBlueMarker` | the node publishes blue 2 5 |

## References

- [5. The marker detector node](../../docs-en/vision/05_marker_detector_node.md)
