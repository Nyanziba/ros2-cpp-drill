# nav02 状態機械とビヘイビアツリー 〔自律制御編〕

試合の状態遷移と、仕事ごとの制御の種類を実装します。

## やること

`next_state` は WaitStart+StartSignal で Search、Search+MarkerSeen で Approach、Approach+Aligned で Align、Align+Placed で Place、Place+Scored で Finished。Approach / Align / Place の MarkerLost は Search。TimeUp は Finished 以外を TimedOut にし、Finished はそのまま。TimedOut と Finished は他のイベントを無視します。書かれていない組み合わせは現状維持。`choose_controller` は MatchPhase が StateMachine、ObstacleAvoidance が BehaviorTree、DriveToZone が StateMachineCallsBehaviorTree。

## 動かしてみる

```bash
./drill run nav02
```

## つまずきポイント

TimeUp を Finished にも適用すると、得点したあとに TimedOut へ落ちます。

## テスト

```bash
./drill run nav02
```

| テスト | 見ているところ |
| --- | --- |
| `PlaysAPointAndStopsOnTime` | 1 点の流れと時間切れ |
| `PicksTheControllerForTheJob` | 制御の選び方 |

## 参考

- [2. 状態機械とビヘイビアツリー](../../docs/autonomy/02_状態機械とビヘイビアツリー.md)
