# cv06 Gaze control [Vision]

Aim a pan-tilt at a detection, and hold still on emergency stop.

## What to do

`update_gaze` prefers the smaller rank (0 game, 1 person, 2 other). At the same rank it keeps the current target. A better rank preempts immediately. A miss keeps the angles and increments lost_frames, and the 5th miss drops the lock. Pan rate is kp*(cx-width/2). Tilt rate is -kp*(cy-height/2). Clamp rates to ±max_rate and angles to the pan and tilt limits. `gaze_xml` is hold when estop is set, track when target_visible is set, and search otherwise. Include a Fallback and a Sequence.

## Run it

```bash
./drill run cv06
```

## Common pitfalls

If you forget that image y points down, tilt moves the wrong way.

## Tests

```bash
./drill run cv06
```

| Test | What it checks |
| --- | --- |
| `ServosTowardGameObject` | rates toward a game object |
| `HigherRankPreemptsAndSameRankDoesNot` | switching by rank |
| `HoldsThroughFlickerThenDrops` | hold until the 5th miss |
| `ClampsPan` | the pan limit |
| `XmlUsesFallbackAndSequence` | the shape of the XML |
| `EstopBeatsTracking` | hold, track, and search |

## References

- [6. Gaze control](../../docs-en/vision/06_gaze_control.md)
