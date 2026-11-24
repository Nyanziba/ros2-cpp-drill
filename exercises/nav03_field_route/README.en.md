# nav03 Routes and recovery [Autonomy]

Implement whether a route fits the match, the frame chain, and the recovery XML.

## What to do

`plan_field_route` is feasible when the list is not empty, no leg_sec is negative, and the total is at most match_time_left (slack 1e-9). Fill in the names and the total even when it is not feasible. `field_frames_connected` needs both direct edges, map to odom and odom to base_link. `slam_mode_name` is `localization` only when have_map is set and keep_mapping is not. Otherwise `mapping`. `nav_recovery_xml` must match the comment in the header exactly, and the Spin line is included only when with_spin is set.

## Run it

```bash
./drill run nav03
```

## Common pitfalls

One wrong space of indentation, or a missing trailing newline, fails RecoveryXmlMatches.

## Tests

```bash
./drill run nav03
```

| Test | What it checks |
| --- | --- |
| `RejectsARouteThatOverrunsTheMatch` | time left |
| `NeedsMapToOdomToBase` | TF and the SLAM mode |
| `RecoveryXmlMatches` | the recovery XML matches exactly |
| `Nav2AndSlamLaunchAreSkipped` | launching them is skipped |

## References

- [3. Routes and recovery](../../docs-en/autonomy/03_routes_and_recovery.md)
