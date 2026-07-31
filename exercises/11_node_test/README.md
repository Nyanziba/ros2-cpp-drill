# 課題 11: テストを書く 〔中級〕

**この課題は他の課題と逆です。あなたが書くのは実装ではなく「テスト」です。**

対応する講習資料: [17_テストとデバッグ.md](../../ros2_lecture/17_テストとデバッグ.md)

## なぜテストを書く課題なのか

17章はこう言っています。

> テスト対象の関数が `rclpy.node.Node` や `rclcpp::Node` を継承していない、ただの関数だということです。
> ROSのノードをテストしようとすると、`rclpy.init()`やノードの起動、executorのspinが必要になり、
> テスト1つ書くだけで一気に重くなります。

この課題では、ロジックをあらかじめ純粋関数（`limit_velocity`）として切り出してあります。
だからこそ、あなたが書くテストは `rclcpp::init()` もノードの起動も spin も一切要りません。
`#include <gtest/gtest.h>` と `#include "drill/velocity_limiter.hpp"` の 2 行で始められます。
これが「ROSに依存しないロジックを普通の関数に切り出す」ことの実利です。

## 題材: 速度指令のリミッタ

`include/drill/velocity_limiter.hpp` に次の関数が宣言されています（実装は非公開）。

```cpp
double limit_velocity(double target, double previous, double max_speed, double max_delta);
```

次の 2 つを**順に**適用した値を返す関数です。

1. **加速度制限**: `previous` から `target` への変化量の絶対値を `max_delta` 以内に収める
2. **速度制限**: 1. の結果の絶対値を `max_speed` 以内に収める

`max_speed` と `max_delta` は 0 以上を想定しており、負の値が来たら 0 として扱う仕様です。

## やること

`test/test_exercise.cpp` の TODO を埋めて、`limit_velocity` のテストを書いてください。
すでに 1 つだけ「通るテスト」（制限にかからない普通のケース）が書いてあります。
まずはこれを読んで、テストの書き方（`limit_velocity` を直接呼んで `EXPECT_DOUBLE_EQ` で
比較するだけ）を掴んでください。

**ただし、この 1 つだけでは合格しません。** 何が足りないかは次の採点の仕組みを読んでください。

## 採点の仕組み: 「通る」だけでは不合格

このテストは 2 つの実装に対して走ります。

| ターゲット | リンクする実装 | 合格条件 |
| --- | --- | --- |
| `test_exercise` | 正しい実装 | **通ること** |
| `test_mutant` | わざとバグを仕込んだ実装 | **落ちること** |

つまり、あなたが書いたテストが正しい実装で通るのは当然として、
**バグ入り実装に対しても同じテストを走らせて、ちゃんと落ちるか**まで見ます。
何もassertしていない空のテストや、正常系しか見ないテストは `test_exercise` は通っても
`test_mutant` も通ってしまうので、不合格です。

これは17章の主張と表裏一体です。「通るテストを書く」のは簡単ですが、
「バグを検出できるテストを書く」のがテストの本当の価値です。落ちないテストは、
そのテストが検証しているつもりの仕様について、実は何も確認していないのと同じです。
今のTODO版のテスト（正常系1つだけ）は `test_exercise` は通りますが `test_mutant` も
通ってしまいます。つまり**まだ不合格の状態**です。

## カバーすべき観点

`test/test_exercise.cpp` の TODO コメントに対応する形で、次の観点をテストしてください。
（期待値そのものは書いてありません。自分で考えて assert してください。）

- **符号**: 速度制限は正の方向だけでなく、負の方向にも効いているか
- **適用順**: 加速度制限してから速度制限、の順で 2 段階に適用されているか
  （一気に大きく動かそうとしたときの挙動で確かめられます）
- **`max_*` に 0 や負の値が来た場合**: 仕様どおり 0 として扱われているか
- **境界値**: 変化量がちょうど `max_delta` と等しい、結果がちょうど `max_speed` と
  等しい、といったギリギリの値でどうなるか

ヒント（バグの種類。答えではなく観点です）:

- 仕込まれているバグの少なくとも 1 つは、**片方の符号（向き）だけ**を確かめる
  テストでは見えて、もう片方の符号では見えない、という性質のものです。
- もう 1 つは、`max_delta` や `max_speed` に**負の値を渡したとき**の扱いに関するものです。
- どのバグも「1 つの観点さえ足せば検出できる」ように作ってあります。
  逆に言うと、観点を 1 つ足すごとに確実に前進します。

## 実物: ノード側は薄いままでいい

この関数を実際に使うノード `VelocityLimiterNode`（`src/velocity_limiter_node.cpp`）も
用意してあります。中身を見ると、ノードがやっているのは
「購読して `limit_velocity()` を呼んで publish するだけ」だとわかります。
計算の本体（境界値や符号の扱い）は全部 `limit_velocity()` 側でテスト済みなので、
ノード自身のテストは「配線が正しいか」だけを見れば足りる、薄いもので済みます。
これが17章の「ロジックを切り出しておけばノード側は薄くできる」の実例です。

## 動かしてみる

`velocity_limiter_node` は `ros2 run` で動かせます。

```bash
source install/setup.bash
ros2 run drill_11_node_test velocity_limiter_node --ros-args -p max_speed:=1.0 -p max_delta:=0.3
```

別の端末で:

```bash
ros2 topic echo /cmd_vel_limited
```

さらに別の端末で:

```bash
ros2 topic pub /cmd_vel_raw std_msgs/msg/Float64 "{data: 5.0}" --once
```

`5.0` を送っても `cmd_vel_limited` 側は `max_delta` と `max_speed` に抑えられた値
（1 回目は `max_delta=0.3` に抑えられて `0.3`、送り続ければ徐々に `max_speed=1.0` へ
近づく）になっているはずです。

## テスト

```bash
./drill run 11
```

（隔離ビルドで手元確認する場合は `colcon build --packages-select drill_11_node_test`
の後、`build/drill_11_node_test/test_exercise` と `build/drill_11_node_test/test_mutant`
を直接実行しても構いません。)

## 参考

- 講習資料: [17_テストとデバッグ.md](../../ros2_lecture/17_テストとデバッグ.md)
- 公式: [ROS 2 Documentation: Jazzy — Testing](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Testing/Testing-Main.html)
