# 2. State machines and behavior trees

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/autonomy/02_状態機械とビヘイビアツリー.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/02_state_machines_and_behavior_trees.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/02_state_machines_and_behavior_trees.html

A three-minute match has a path with known places to return to: start, search, approach, place, finish. That fits a state machine. Obstacle avoidance, which every cycle asks what matters most right now, fits a behavior tree. Driving to a zone ticks a tree from inside a state.

## Transitions

WaitStart plus StartSignal is Search. Search plus MarkerSeen is Approach. Approach plus Aligned is Align. Align plus Placed is Place. Place plus Scored is Finished. MarkerLost from Approach, Align, or Place returns to Search. TimeUp moves every state except Finished to TimedOut. TimeUp on Finished stays Finished. TimedOut and Finished ignore other events.

| Item | Rule |
| --- | --- |
| A match phase | StateMachine |
| Obstacle avoidance | BehaviorTree |

## Which one

`choose_controller` maps MatchPhase to a state machine, ObstacleAvoidance to a behavior tree, and DriveToZone to a state machine that calls a behavior tree. The point of the chapter is not to force everything into one of them.

```cpp
MatchState state = next_state(current, event);
Controller which = choose_controller(task);
```

## What CI does not grade

CI connects neither a referee signal nor a clock. It passes one event at a time and checks the next state.

## Matching exercise

```bash
./drill run nav02
```
