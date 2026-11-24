# bt02 アクションと黒板 〔ビヘイビアツリー編〕

DriveClient にゴールを送り、状態番号を NodeStatus に写します。

## やること

`make_drive_goal` は time_left < 0.5 なら zone `abort` と timeout 0。それ以外の timeout は min(3.0, time_left) で zone はそのまま。`status_from_action` は 2 と 3 が RUNNING、4 が SUCCESS、それ以外が FAILURE。木は黒板 `drive_client` と `time_left`、ポート zone を使います。onStart で送り RUNNING。abort なら送らず FAILURE。onHalted は cancel。

## 動かしてみる

```bash
./drill run bt02
```

## つまずきポイント

abort のときも send すると、DoesNotSendWhenTimeIsAlmostUp が失敗します。

## テスト

```bash
./drill run bt02
```

| テスト | 見ているところ |
| --- | --- |
| `CapsTimeoutAndAbortsLate` | timeout と abort |
| `MapsActionStatus` | 状態番号 |
| `SendsGoalThenSucceeds` | 送って成功 |
| `DoesNotSendWhenTimeIsAlmostUp` | 残り僅かでは送らない |
| `HaltCancels` | halt で cancel |

## 参考

- [2. アクションと黒板](../../docs/behavior-tree/02_アクションと黒板.md)
