# 課題 07: パラメータの変更を監視する 〔中級〕

公式チュートリアル
[Monitoring for parameter changes (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Monitoring-For-Parameter-Changes-CPP.html)
の `SampleNodeWithParameters` をそのまま書きます。

## やること

`src/node_with_parameters.cpp` の TODO を埋めてください。仕様は公式チュートリアルと同一です。

| 項目 | 値 |
| --- | --- |
| ノード名 | `node_with_parameters` |
| パラメータ名 | `an_int_param` |
| 型 | 整数 |
| 既定値 | `0` |
| ログ | `cb: Received an update to parameter "an_int_param" of type integer: "<値>"` |

クラス宣言（`include/drill/node_with_parameters.hpp`）は与えてあります。メンバ変数
`param_subscriber_` / `cb_handle_` に何を入れるかと、コールバックの中身を考えてください。

## 課題05との使い分け（ポーリング vs イベント通知）

課題06（`Using parameters in a class`）では、タイマのコールバックが毎回
`get_parameter()` を呼んで値を読み直す「ポーリング」方式でした。パラメータが
変わっていようがいまいが、決まった周期で読みに行きます。

今回の `ParameterEventHandler` は逆で、`/parameter_events` トピックを購読し、
**実際に値が変わった時だけ**コールバックが呼ばれる「イベント通知」方式です。

どちらを使うべきかの目安:

- **ポーリング（課題06）** — もともと一定周期で処理するループがあり、そのついでに
  最新の値を使いたいだけの場合。実装が単純で、購読やハンドルの寿命管理も不要。
- **イベント通知（課題07）** — パラメータが変わった瞬間に何かを実行したい場合
  （再設定処理を走らせる、他ノードの動的パラメータを監視するなど）。
  変化がなければコールバックは呼ばれないので、無駄な読み出しが発生しない。
  一方で `ParameterEventHandler` と `ParameterCallbackHandle` のオブジェクトを
  生存させ続ける責任が増える。

## 動かしてみる

テストが通ったら、公式チュートリアルと同じように手で動かせます。

```bash
source install/setup.bash
ros2 run drill_07_param_events parameter_event_handler
```

別の端末で:

```bash
ros2 param set /node_with_parameters an_int_param 43
```

さらに別の端末で `/parameter_events` を覗くと、上のコマンドが実際に
イベントとして流れているのが見えます。

```bash
ros2 topic echo /parameter_events
```

`parameter_event_handler` を実行している端末には
`cb: Received an update to parameter "an_int_param" of type integer: "43"`
というログが出るはずです。

## つまずきポイント

- **`add_parameter_callback()` の戻り値をどうするか。** これが今回いちばんの
  ハマりどころです。戻り値（ハンドル）をどこにも保持せず一時オブジェクトの
  ままにしてしまうと、登録した直後にコールバックが解除されてしまいます。
  このクラスには、そのために使えそうなメンバがもう用意されています。
- コールバックは `/parameter_events` を経由して届きます。ノードを spin
  していなければ、パラメータをいくら `set_parameter()` しても呼ばれません。
- `param_subscriber_` の戻り値（`std::make_shared<rclcpp::ParameterEventHandler>(this)`）
  も同様にメンバへ保持すること。ローカル変数で受けるとコンストラクタを
  抜けた時点で破棄されます。

## 公式との違い

- 公式チュートリアルはクラスと同じファイルに `main()` を書きますが、テストから
  クラスを直接使えるように `src/parameter_event_handler_main.cpp` に分けています。
- 公式のクラス名は `SampleNodeWithParameters` ですが、この課題では
  `NodeWithParameters` にしています（ノード名 `node_with_parameters` は共通）。
- テスト用に、公式には無いメンバ `latest_value_` と、公開アクセサ
  `latest_value()` を追加しています。コールバックが実際に呼ばれ、渡された値を
  受け取れたことをテストから確認するための仕掛けです（コールバックの中身自体は
  公式と同じログを出しつつ、加えて `latest_value_` に値を控えます）。

## テスト

```bash
./drill run 07
```

| テスト | 見ているところ |
| --- | --- |
| `an_int_paramが整数型で既定値0で宣言されている` | `declare_parameter()` の型と既定値 |
| `an_int_paramをsetするとlatest_valueが更新される` | コールバックが実際に呼ばれているか |
| `公式と同じcbログを出している` | `RCLCPP_INFO` の書式 |
| `2回目の変更でもコールバックが呼ばれる` | ハンドルを保持し続けられているか |

## 参考

- 公式: [Monitoring for parameter changes (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Monitoring-For-Parameter-Changes-CPP.html)
- 課題06: [../05_parameters/README.md](../05_parameters/README.md)（ポーリング方式との比較）
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
