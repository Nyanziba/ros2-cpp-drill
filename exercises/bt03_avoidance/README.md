# bt03 回避 〔ビヘイビアツリー編〕

距離から AvoidCommand を決め、同じ結果を木でも書きます。

## やること

`decide_avoidance` は front < 150 で Stop。それ以外で front < 400 なら、left >= right のとき DodgeLeft、そうでなければ DodgeRight。障害物が無く goal_visible なら Go、なければ Search。木は黒板 front_mm, left_mm, right_mm, goal_visible を読み、command を stop / left / right / go / search にします。XML に Fallback と Sequence を含めます。

## 動かしてみる

```bash
./drill run bt03
```

## つまずきポイント

関数だけ直して木を直さないと、MatchesTheFunction が失敗します。

## テスト

```bash
./drill run bt03
```

| テスト | 見ているところ |
| --- | --- |
| `StopsBeforeDodging` | 停止が先 |
| `MatchesTheFunction` | 木と関数が一致 |

## 参考

- [3. 回避](../../docs/behavior-tree/03_回避.md)
