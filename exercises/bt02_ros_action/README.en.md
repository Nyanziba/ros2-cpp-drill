# bt02 Actions and the blackboard [Behavior trees]

Send a goal through DriveClient and map the status code to NodeStatus.

## What to do

`make_drive_goal` uses zone `abort` and timeout 0 when time_left < 0.5. Otherwise the timeout is min(3.0, time_left) and the zone is unchanged. `status_from_action` maps 2 and 3 to RUNNING, 4 to SUCCESS, and anything else to FAILURE. The tree uses blackboard `drive_client` and `time_left`, plus the zone port. onStart sends and returns RUNNING. abort does not send and returns FAILURE. onHalted cancels.

## Run it

```bash
./drill run bt02
```

## Common pitfalls

Sending on abort fails DoesNotSendWhenTimeIsAlmostUp.

## Tests

```bash
./drill run bt02
```

| Test | What it checks |
| --- | --- |
| `CapsTimeoutAndAbortsLate` | timeout and abort |
| `MapsActionStatus` | status codes |
| `SendsGoalThenSucceeds` | send, then succeed |
| `DoesNotSendWhenTimeIsAlmostUp` | do not send when almost no time is left |
| `HaltCancels` | halt cancels |

## References

- [2. Actions and the blackboard](../../docs-en/behavior-tree/02_actions_and_blackboard.md)
