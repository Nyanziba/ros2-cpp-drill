# nav02 State machines and behavior trees [Autonomy]

Implement the match transitions and which controller each job uses.

## What to do

`next_state`: WaitStart+StartSignal is Search, Search+MarkerSeen is Approach, Approach+Aligned is Align, Align+Placed is Place, Place+Scored is Finished. MarkerLost from Approach, Align, or Place is Search. TimeUp moves every state except Finished to TimedOut, and Finished stays Finished. TimedOut and Finished ignore other events. Any other pair stays put. `choose_controller` maps MatchPhase to StateMachine, ObstacleAvoidance to BehaviorTree, and DriveToZone to StateMachineCallsBehaviorTree.

## Run it

```bash
./drill run nav02
```

## Common pitfalls

Applying TimeUp to Finished drops a scored match into TimedOut.

## Tests

```bash
./drill run nav02
```

| Test | What it checks |
| --- | --- |
| `PlaysAPointAndStopsOnTime` | one point, then the clock |
| `PicksTheControllerForTheJob` | which controller |

## References

- [2. State machines and behavior trees](../../docs-en/autonomy/02_state_machines_and_behavior_trees.md)
