# ROS2講習03: turtlesimとrqtで感覚を掴む

## はじめに

前回、[02_環境構築](02_環境構築.md)でROS 2 Jazzyが動く環境を作りました。今回はコードを一行も書かずに、ROS 2のノード・トピックという概念を目で見て理解します。この講習が終わると、亀を動かしながら「今なにが起きているか」をrqtで確認できるようになります。

## 講習目標 / 講習の進め方

- 対象: [02_環境構築](02_環境構築.md)まで完了している新入生
- 所要時間: 30〜40分
- 前提知識: ターミナル操作の基礎（コピペで進められます）
- ゴール: turtlesimを操作し、rqt_graphでノードとトピックのつながりを確認できる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jalisco がセットアップ済みの端末（各自のノートPCでよい）
- ターミナルを2枚以上並べて開ける画面サイズ（外部モニタがあると楽）
- `ros-jazzy-rqt*` 系パッケージのインストール可否はネットワーク速度に依存するため、事前に各端末でインストールを済ませておくと当日の時間を圧迫しない

### 口頭試問

Q1. `ros2 topic pub`で亀を動かしたあと、そのターミナルでCtrl+Cを押すと亀はどうなりますか。

<details><summary>模範解答</summary>
pubは一度きりのメッセージ配信ではなく、デフォルトで一定周期（1Hz）で繰り返し配信し続けています。Ctrl+Cで配信が止まると、亀への速度指令が途絶えるので亀はその場で停止します。turtlesimは慣性で動き続けるような実装にはなっていません。
</details>

Q2. teleop_keyでキーを押しても亀が反応しないとき、まず確認すべきことは何ですか。

<details><summary>模範解答</summary>
teleop_keyを起動したターミナルウィンドウにフォーカス（クリックしてアクティブ）が当たっているかを確認します。teleopはキーボードイベントをそのターミナルプロセスで直接読んでいるため、別のウィンドウ（rqtやブラウザなど）がアクティブなままだと入力が飛びません。
</details>

Q3. rqt_graphに表示される矢印は何を表していますか。ノード同士が直接関数呼び出しをしているのですか。

<details><summary>模範解答</summary>
矢印はトピック通信を表しています。ノードは互いを直接呼び出しているのではなく、DDS（Data Distribution Service）経由でトピックにpublish/subscribeしているだけです。ノード同士は互いの存在を知らなくてもよく、トピック名と型が一致していれば通信が成立します。この疎結合な構造がROS 2の設計の中心です。
</details>

### 時間配分の目安

| 項目 | 時間 |
|---|---|
| turtlesimのインストールと起動 | 5分 |
| teleopで操作 | 5分 |
| rqtとrqt_graphの観察 | 10分 |
| cmd_velの中身を見る | 10分 |
| Service Callerでspawn | 5分 |
| 課題・口頭試問 | 5〜10分 |

## 本文

### 課題1: turtlesimをインストールして起動する

学習内容: ROS 2のパッケージはaptで配布されており、`ros2 run`で実行できる。

準備: [02_環境構築](02_環境構築.md)でROS 2 Jazzyのaptリポジトリが設定済みであること。

内容:

```bash
sudo apt update
sudo apt install ros-jazzy-turtlesim
```

インストールが終わったら、ノードを起動します。

```bash
ros2 run turtlesim turtlesim_node
```

画面に亀（矢印のシンプルな図形です、本物の亀の絵ではありません）が表示されたウィンドウが開きます。このウィンドウは`turtlesim_node`というノードが描画しているものです。ターミナルにはノード名やトピック名がログとして流れます。このターミナルは占有されるので、次の操作には別のターミナルを新しく開きます。

ヒント: `ros2 run <パッケージ名> <実行可能ファイル名>`が基本形です。パッケージ名と実行可能ファイル名は別物で、1つのパッケージが複数の実行可能ファイルを持つこともあります。

### 課題2: teleop_keyで亀を操作する

学習内容: キーボード入力をトピックに変換して送るノードがteleopです。

準備: 課題1のturtlesim_nodeを起動したままにしておく。

内容: 新しいターミナルを開いて次を実行します。

```bash
ros2 run turtlesim turtle_teleop_key
```

矢印キーを押すと亀が動きます。動かない場合は、teleop_keyを起動したターミナルウィンドウをクリックしてフォーカスを移してから試してください。**teleopはターミナルウィンドウそのものにフォーカスがないとキー入力を受け取れません**。turtlesimの描画ウィンドウをクリックしても効きません。

Ctrl+Cで停止すると、その瞬間から亀は指令を受け取らなくなり停止します。

ヒント: 矢印キー以外にも、`q`/`e`で回転速度の変更、`Q`/`E`で並進速度の変更ができます（teleop_key起動時のターミナル出力に一覧が出ます）。

### 課題3: rqtとrqt_graphでノードのつながりを見る

学習内容: rqt_graphはROS 2システム内のノードとトピックの関係をグラフで可視化するツールです。

準備: turtlesim_node、turtle_teleop_keyの両方を起動したままにする。

内容: 3枚目のターミナルで次を実行します。

```bash
sudo apt install ros-jazzy-rqt ros-jazzy-rqt-graph ros-jazzy-rqt-common-plugins
rqt_graph
```

ウィンドウが開き、ノードとトピックの関係が矢印で描かれます。`/turtlesim`ノードと`/teleop_turtle`ノードの間に、`/turtle1/cmd_vel`というトピックを介した矢印が見えるはずです。矢印の向きは`teleop_turtle`から`turtlesim`への一方向で、teleopが送り、turtlesimが受け取っていることを表しています。

同じ内容はコマンドラインからも確認できます。

```bash
ros2 node list
ros2 node info /turtlesim
ros2 topic list
```

`ros2 node info /turtlesim`を打つと、そのノードがどのトピックをSubscribe/Publishしているか、どのサービスを提供しているかが一覧で出ます。rqt_graphは、このテキスト情報を図にしただけのものだと考えると理解が早いです。

ヒント: rqt_graph左上の更新ボタン（回転する矢印アイコン）を押さないと、新しく起動したノードが反映されないことがあります。

### 課題4: /turtle1/cmd_velの正体を見る

学習内容: teleopが送っているトピックの型と内容を実際に確認する。これは後の講習で実機のロボットに送る`cmd_vel`と全く同じ構造です。

準備: turtlesim_node、turtle_teleop_keyを起動したまま。

内容: トピックの型を確認します。

```bash
ros2 topic info /turtle1/cmd_vel
```

`geometry_msgs/msg/Twist`という型が表示されます。この型の構造を見てみます。

```bash
ros2 interface show geometry_msgs/msg/Twist
```

出力は次のようになります。

```
Vector3  linear
        float64 x
        float64 y
        float64 z
Vector3  angular
        float64 x
        float64 y
        float64 z
```

並進速度（linear）と角速度（angular）をそれぞれxyz3軸で表現しているだけの、単純な構造です。teleop_keyで矢印キーの上下を押すとlinear.xが、左右を押すとangular.zが変化しています。

実際に流れている値を覗いてみます。

```bash
ros2 topic echo /turtle1/cmd_vel
```

このコマンドを実行したまま別のターミナルでteleopを操作すると、キーを押した瞬間の値がターミナルに流れます。何も押していないときは何も表示されません（teleopは変化があったときだけ送っているためです）。

**このトピック名と型は、後にNav2やmove_base_flex経由で実機のロボットに送る`cmd_vel`とまったく同じ構造です**。turtlesimは平面上の亀を動かしているだけですが、通信の形はロボコンの実機と同一です。ここで見たものは他の記事でも何度も出てきます。

ヒント: `ros2 topic echo`は他のノードが送る値を横から覗いているだけで、自分から何かを送ってはいません。誰かが送っていなければ何も表示されないままです。

### 課題5: rqtのService Callerで亀をspawnする

学習内容: トピックとは別にサービスという通信の形があり、rqtからGUIで呼び出せる。

準備: turtlesim_nodeを起動したまま。

内容: rqtを起動します。

```bash
rqt
```

メニューから「Plugins」→「Services」→「Service Caller」を選びます。プルダウンから`/spawn`を選択すると、`turtlesim/srv/Spawn`のリクエスト項目（x, y, theta, name）が入力欄として表示されます。適当な座標（例: x=3.0, y=3.0, theta=0.0）を入れて「Call」ボタンを押すと、turtlesimの画面にもう1匹亀が現れます。

`ros2 service list`を打つと`/spawn`をはじめとするサービス一覧が見えます。トピックが「配信し続ける一方通行の通信」なのに対し、サービスは「呼び出して一度だけ結果を受け取るリクエスト・レスポンス型の通信」です。spawnのように「1回だけ実行して結果を確認したい」操作にサービスが向いていることが、ここで体感できます。

ヒント: Service Callerの入力欄が空のまま「Call」を押すとデフォルト値（0.0など）で呼ばれます。同じ名前で2回spawnしようとするとエラーになります（名前は重複できません）。

## 発展

rqt_graphで見えた矢印は、すべてトピック通信です。今回は「見て確認する」だけでしたが、次回の[04_ノード](04_ノード.md)ではノード自体の構造、その次の記事ではこのトピックを自分のプログラムからpublish/subscribeする方法を扱います。turtlesimで見た`/turtle1/cmd_vel`が、自分の書いたコードから流れる感覚をそこで掴んでください。

サービスについても、今回はGUIから呼ぶだけでしたが、後の講習でサーバ・クライアントを自分で実装します。

## おわりに

turtlesimは見た目がおもちゃっぽいですが、ここで見たノード・トピック・サービスという概念は実際のロボット制御まで一貫して使われています。焦らず、rqt_graphで矢印がどう動くかを何度も眺めてみてください。わからなければ先輩に聞きましょう。

## 資料

- 対応する公式チュートリアル: [Introducing turtlesim and rqt (ROS 2 Jazzy)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Introducing-Turtlesim/Introducing-Turtlesim.html)
- [Understanding topics](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.html)
- [Understanding services](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Services/Understanding-ROS2-Services.html)
- 前回: [02_環境構築](02_環境構築.md)
- 次回: [04_ノード](04_ノード.md)
