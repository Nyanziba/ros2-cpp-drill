# ROS2講習09: launchとros2 bag

## はじめに

これまでの講習では、ノードを1つずつ`ros2 run`で起動してきました。今回はそれをやめます。複数ノードをまとめて起動する`launch`と、通信内容を丸ごと記録・再生する`ros2 bag`を扱います。この講習が終わると、1コマンドで複数ノードを立ち上げ、実行中の通信を記録して後から再生できるようになります。第1部の最後の記事です。

## 講習目標 / 講習の進め方

- 対象: [08_アクション](08_アクション.md)まで完了している新入生
- 所要時間: 55〜70分
- 前提知識: `ros2 run`でのノード起動、トピック・cmd_velの基本（03, 05で扱った内容）
- ゴール: Python launchファイルで複数ノードを起動できる。ros2 bagでトピックを記録し、再生して同じ動きを再現できる。さらにlaunchにbag記録を組み込んで「起動すれば必ず記録される」状態を作れる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jalisco がセットアップ済みの端末
- `ros-jazzy-turtlesim`（03で導入済みのはず）
- `ros-jazzy-rosbag2*`系パッケージ（Jazzyのデスクトップ変種インストールなら通常同梱。`ros2 bag --help`が通るか事前確認しておく）
- ターミナルを3枚以上並べて開ける画面

### 口頭試問

Q1. launchファイルを使わずに`ros2 run`をターミナルごとに叩いていく方法と比べて、launchファイルの利点は何ですか。

<details><summary>模範解答</summary>
複数ノードを1コマンドでまとめて起動でき、ノード名のremapやパラメータの注入もファイル1つに記述できます。本番のロボットは10ノードを超えることが普通で、ターミナルを10枚開いて手で起動していくのは現実的ではありません。起動漏れや起動順のミスも減らせます。
</details>

Q2. `ros2 bag record -a`と、トピックを指定して記録する方法の違いと、それぞれをいつ使うべきか説明してください。

<details><summary>模範解答</summary>
`-a`は起動中の全トピックを記録します。何が問題の原因かわからないテストラン後の解析には`-a`が安全です（後から見たいトピックが記録されていないと詰みます）。一方、対象トピックが最初からわかっている場合や、画像・点群のような大容量トピックを含む場合は、トピック名を指定して記録すると容量と後の解析の手間を抑えられます。
</details>

Q3. `ros2 bag play`で再生したcmd_velと、実際にteleopから流していたcmd_velで、亀の動きに違いは出ますか。

<details><summary>模範解答</summary>
基本的に同じ動きが再現されます。bagはトピックに流れたメッセージとそのタイムスタンプをそのまま記録しており、再生時は記録時と同じ間隔でメッセージを流すからです。turtlesimはcmd_velの値だけを見て動くので、送信元がteleopかbag再生かは区別しません。ただし記録漏れのトピック（他ノードの内部状態など）に依存する処理があれば、そこは再現されません。
</details>

### 時間配分の目安

| 項目 | 時間 |
|---|---|
| launchの必要性の説明 | 5分 |
| turtlesim+mimicのlaunchファイル作成・実行 | 15分 |
| ros2 bag record/info/play | 15分 |
| cmd_vel記録・再生課題 | 10分 |
| launchにbag記録を組み込む（課題5） | 15分 |
| 口頭試問 | 5〜10分 |

## 本文

### 課題1: なぜlaunchが必要なのか

学習内容: ノード数が増えると`ros2 run`の逐次起動が破綻する。launchはこれを1ファイルにまとめる仕組みです。

準備: 特になし。

内容: これまでの講習では、turtlesimなら`turtlesim_node`と`turtle_teleop_key`の2つを別々のターミナルで起動していました。2つならまだ手で起動できます。しかし実際のロボットは、モータ制御ノード、センサドライバ、状態推定ノード、経路計画ノード、経路追従ノード、状態管理ノードなど、実機起動時に10ノードを超えることが普通です。ターミナルを10枚開いて順番を守って1つずつ起動する運用は、起動漏れ・起動順ミス・タイポの温床になります。

launchは、起動したいノードの一覧・remap・パラメータをPythonファイル（またはXML/YAML）に記述し、`ros2 launch`で一括起動する仕組みです。今回はPython形式を扱います。ROS 2の公式launch APIがPythonで完結しており、条件分岐や変数展開など複雑な起動ロジックを組みやすいためです。

ヒント: launchファイルは「ノードをどう起動するかの設定ファイル」であり、それ自体はロボットの制御ロジックを書く場所ではありません。ロジックは各ノードの中に書きます。

### 課題2: turtlesim2匹とmimicノードをlaunchで起動する

学習内容: Python launchファイルの最小構成を書き、複数ノードを1コマンドで起動する。

準備: ワークスペースはまだ作っていないので、任意の作業ディレクトリで構いません（正式なパッケージ化は[10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)以降で扱います）。

内容: 作業用ディレクトリを作り、launchファイルを置きます。

```bash
mkdir -p ~/ros2_lecture/launch
cd ~/ros2_lecture/launch
```

`mimic_launch.py`を次の内容で作成します。これはROS 2公式チュートリアルの例そのもので、turtlesimを2匹起動し、1匹（turtlesim2）が別の1匹（turtlesim1）の動きをそっくり真似する`mimic`ノードを繋ぎます。

```python
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='turtlesim',
            namespace='turtlesim1',
            executable='turtlesim_node',
            name='sim',
        ),
        Node(
            package='turtlesim',
            namespace='turtlesim2',
            executable='turtlesim_node',
            name='sim',
        ),
        Node(
            package='turtlesim',
            executable='mimic',
            name='mimic',
            remappings=[
                ('/input/pose', '/turtlesim1/turtle1/pose'),
                ('/output/cmd_vel', '/turtlesim2/turtle1/cmd_vel'),
            ],
        ),
    ])
```

`generate_launch_description()`という関数名は固定です。`ros2 launch`はこの関数を呼んでノードの一覧（`LaunchDescription`）を受け取ります。`Node`1つが1ノードの起動設定に対応し、`namespace`で名前空間を分けることで同じ`turtlesim_node`を2つ同時に起動しても名前がぶつかりません。`remappings`はトピック名の付け替えで、mimicノードがデフォルトで使う`/input/pose`と`/output/cmd_vel`を、実際に存在するturtlesim1・turtlesim2側のトピック名に接続しています。

実行します。

```bash
ros2 launch ~/ros2_lecture/launch/mimic_launch.py
```

turtlesimのウィンドウが2つ開きます。別のターミナルでturtlesim1側にteleopをつなぎます。

```bash
ros2 run turtlesim turtle_teleop_key --ros-args --remap __ns:=/turtlesim1
```

矢印キーで亀1匹（turtlesim1側）を動かすと、もう1匹（turtlesim2側）が同じ動きをします。teleopが送っているのは`/turtlesim1/turtle1/cmd_vel`ですが、mimicノードが`/turtlesim1/turtle1/pose`を読み取って`/turtlesim2/turtle1/cmd_vel`に変換して送っているので、動きが伝播します。1コマンド（`ros2 launch`）で3ノードが立ち上がったことを確認してください。

ヒント: `ros2 node list`を打つと`/turtlesim1/sim`、`/turtlesim2/sim`、`/mimic`の3ノードが見えます。namespaceがノード名の前に付いていることを確認しましょう。

### 課題3: ros2 bagで記録する

学習内容: `ros2 bag record`は指定したトピック（または全トピック）のメッセージをファイルに記録する。

準備: 課題2のturtlesim1・turtlesim2・mimicを起動したままにしておきます（teleopは止めてよい）。

内容: 記録用のディレクトリに移動し、記録を開始します。まずは全トピックを記録する方法です。

```bash
mkdir -p ~/ros2_lecture/bags
cd ~/ros2_lecture/bags
ros2 bag record -a -o all_topics_bag
```

`-a`は起動中の全トピックを対象にする指定、`-o`は出力先ディレクトリ名の指定です。Ctrl+Cで記録を止めます。

なお**Jazzyでは記録形式のデフォルトが`mcap`です**（`ros2 bag record --help`に`-s {mcap,sqlite3} ... defaults to 'mcap'`と出ます）。Humble以前の資料やネットの記事では`.db3`（sqlite3）ができる前提で書かれているものが多いので、出力ファイルの拡張子が違っても慌てないでください。どうしてもsqlite3で残したい場合は`-s sqlite3`を付けます。

トピックを指定して記録する場合は次のようにします。
```bash
ros2 bag record -o cmd_vel_only_bag --topics /turtlesim1/turtle1/cmd_vel
```

記録したbagの内容を確認します。

```bash
ros2 bag info cmd_vel_only_bag
```

記録時間、メッセージ数、記録されたトピック名と型の一覧が表示されます。

ヒント: `-a`で記録すると、画像や点群を含む環境ではファイルサイズが一気に大きくなります。何を記録すべきかわかっている場合はトピック指定が安全です。何が起きたかわからないテストラン直後は、後から見たいトピックが記録されていない事態を避けるため`-a`を使うべきです。

### 課題4: cmd_velを記録して再生する

学習内容: 記録したcmd_velを`ros2 bag play`で再生すると、記録時と同じ動きが再現される。

準備: turtlesim1・turtlesim2・mimicを起動したままにしておく。teleopも起動して操作できる状態にする。

内容: cmd_velだけを記録します。

```bash
cd ~/ros2_lecture/bags
ros2 bag record -o teleop_cmd_vel --topics /turtlesim1/turtle1/cmd_vel
```

記録を開始したら、別ターミナルのteleopで亀を10秒ほど適当に動かします。上下左右のキーを何度か押して、亀が動いたことを確認してください。動かし終えたら記録側のターミナルでCtrl+Cを押して記録を止めます。

亀の位置をリセットするため、turtlesimを再起動するか、`/reset`サービスを呼びます。

```bash
ros2 service call /reset std_srvs/srv/Empty
```

teleopを止めて（Ctrl+C）、記録したbagを再生します。

```bash
ros2 bag play teleop_cmd_vel
```

亀が、さっきteleopで操作したのと同じ軌跡で動きます。teleopのウィンドウをクリックして触っていなくても、cmd_velにメッセージが流れ続けているのが`ros2 topic echo /turtlesim1/turtle1/cmd_vel`で確認できます。

課題として、以下を自分で確認してください。

1. `ros2 bag play`実行中に`ros2 topic hz /turtlesim1/turtle1/cmd_vel`を叩き、記録時とだいたい同じ配信周波数で再生されていることを確認する
2. 再生を`ros2 bag play -r 2.0 teleop_cmd_vel`のように2倍速で試し、亀の動きが速くなることを確認する

<details><summary>解答</summary>
1. teleopはキー入力があったときのみメッセージを送るため、`hz`は入力の頻度に依存します。記録時に押していたキーの頻度と近い値が再生時にも出れば、タイムスタンプ通りに再生されていることになります。
2. `-r`（rate）オプションは再生速度の倍率です。2.0を指定すると、記録されたメッセージ間の時間間隔が半分になり、亀は同じ経路をより短時間でなぞります。動きの形（軌跡）自体は変わりません。
</details>

ヒント: bagはトピックに流れた生のメッセージとタイムスタンプを記録しているだけです。再生時、bag playは記録されたノード（この場合teleop）を復元しているわけではなく、記録された通りにトピックへメッセージを流し直しているだけだという点を理解しておくと、後々の「bagで何が再現できて何が再現できないか」の判断がしやすくなります。

### 課題5: launchにbag記録を組み込む

学習内容: `ExecuteProcess`でlaunchファイルからbag記録を起動し、「起動したら必ず記録が走る」状態を作る。

準備: 課題2の`mimic_launch.py`と、課題3・4の`ros2 bag`コマンドが両方動いていること。

内容:

ここまでは、launchでノードを起動し、**別のターミナルで手で**`ros2 bag record`を打っていました。この運用には構造的な問題があります。

**録り忘れます。**

テストランの直前は慌ただしく、「起動 → 走らせる」で手一杯になります。そして不具合が起きたあとで「bagを録っていなかった」と気付きます。おわりに節で「テストランではbagを必ず走らせる」と書きますが、**人間の規律に頼る対策は失敗します。** launchに組み込んでしまえば、起動すれば必ず記録されます。

`ros2 bag record`はノードではなくコマンドなので、`Node`ではなく`ExecuteProcess`を使います。課題2の`mimic_launch.py`に4つめの要素を足します。ファイル名を`mimic_record_launch.py`として作成してください。

```python
from launch import LaunchDescription
from launch.actions import ExecuteProcess
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='turtlesim',
            namespace='turtlesim1',
            executable='turtlesim_node',
            name='sim',
        ),
        Node(
            package='turtlesim',
            namespace='turtlesim2',
            executable='turtlesim_node',
            name='sim',
        ),
        Node(
            package='turtlesim',
            executable='mimic',
            name='mimic',
            remappings=[
                ('/input/pose', '/turtlesim1/turtle1/pose'),
                ('/output/cmd_vel', '/turtlesim2/turtle1/cmd_vel'),
            ],
        ),
        ExecuteProcess(
            cmd=[
                'ros2', 'bag', 'record',
                '-o', 'mimic_bag',
                '--topics',
                '/turtlesim1/turtle1/cmd_vel',
                '/turtlesim1/turtle1/pose',
            ],
            output='screen',
        ),
    ])
```

`Node`と`ExecuteProcess`の違いは単純です。`Node`は「パッケージ名と実行ファイル名」を指定してROSノードを起動する専用のアクション、`ExecuteProcess`は「任意のコマンド」を`cmd`のリストで起動する汎用のアクションです。`ros2 bag record`はROSノードではなくCLIコマンドなので後者を使います。

実行します。

```bash
cd ~/ros2_lecture/bags
ros2 launch ~/ros2_lecture/launch/mimic_record_launch.py
```

ログにrecorderの行が混ざります。`[ros2-4]`が4番目の要素（`ExecuteProcess`）の出力という意味です。

```
[ros2-4] [INFO] [rosbag2_recorder]: Starting recording to 'mimic_bag'
[ros2-4] [INFO] [rosbag2_recorder]: Listening for topics...
[ros2-4] [INFO] [rosbag2_recorder]: Recording...
[ros2-4] [INFO] [rosbag2_recorder]: Subscribed to topic '/turtlesim1/turtle1/pose'
```

10秒ほど放置してから、launchのターミナルで**Ctrl+Cを1回**押してください。recorderもちゃんと終了処理をします。

```
[ros2-4] [INFO] [rosbag2_recorder]: Pausing recording.
[mimic-3] [INFO] [rclcpp]: signal_handler(SIGINT/SIGTERM)
[turtlesim_node-2] [INFO] [rclcpp]: signal_handler(SIGINT/SIGTERM)
[turtlesim_node-1] [INFO] [rclcpp]: signal_handler(SIGINT/SIGTERM)
[ros2-4] [INFO] [rosbag2_recorder]: Recording stopped
[ros2-4] [INFO] [rosbag2_recorder]: Event publisher thread: Exited
```

**Ctrl+Cがlaunchから全プロセスへ伝わり、bagは正しく閉じられます。** 確認します。

```bash
ros2 bag info mimic_bag
```

```
Files:             mimic_bag_0.mcap
Bag size:          43.7 KiB
Duration:          8.992216267s
Messages:          563
Topic information: Topic: /turtlesim1/turtle1/pose | Type: turtlesim/msg/Pose | Count: 563
```

`/turtlesim1/turtle1/pose`が563件記録されています。**`/turtlesim1/turtle1/cmd_vel`は記録対象に指定したのに出てきません。** teleopを起動していないので、そのトピックに1件もメッセージが流れなかったからです。

これは覚えておく価値のある挙動です。**`ros2 bag info`に出てこないトピックは「記録に失敗した」のではなく「そもそも流れていなかった」可能性がある**ということです。テストラン後にbagを見て「cmd_velが無い」と焦る前に、publisher側が動いていたかを疑ってください。

課題として、以下を自分で確認してください。

1. teleopを起動した状態で同じlaunchを実行し、`cmd_vel`も記録されることを確認する
2. `-o mimic_bag`を変えずに2回目を実行し、何が起きるかを確認する

<details><summary>解答</summary>

1. `ros2 bag info`のTopic informationに`/turtlesim1/turtle1/cmd_vel | Type: geometry_msgs/msg/Twist`の行が増えます。teleopでキーを押した回数だけCountが増えるので、`pose`（一定周期で流れる）と違ってCountが小さい値になります。

2. エラーで記録が始まりません。

```
[ERROR] [ros2bag]: Output folder 'mimic_bag' already exists.
```

**bagは同名の出力先が既にあると上書きせず失敗します。** これは事故防止として正しい挙動ですが、launchに組み込むと「2回目の起動が静かに記録なしで走る」ことになりかねません。実運用では出力先にタイムスタンプを入れます。

```python
from launch.substitutions import LocalSubstitution
# あるいは Python の側で datetime を使って名前を作る
```

素直なのはlaunchファイルの中でPythonとして名前を組むことです。`generate_launch_description()`は普通のPython関数なので、これがそのまま動きます。

```python
import datetime

stamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
# ... cmd=['ros2', 'bag', 'record', '-o', f'run_{stamp}', '--topics', ...]
```

</details>

ヒント: 3点、実運用で引っかかるところを挙げます。

**出力先は`ros2 launch`を実行したディレクトリからの相対パス**です。launchファイルが置いてある場所ではありません。`-o mimic_bag`のように相対パスで書くと、どこで`ros2 launch`を打ったかによってbagの出来る場所が変わります。迷うなら絶対パスにしてください。

**`--topics`を付けてください。** トピック名を位置引数として並べる書き方（`ros2 bag record -o foo /topic_a`）も動きますが、Jazzyでは非推奨の警告が出ます。

```
[WARN] [ros2bag]: Positional "topics" argument deprecated. Please use optional "--topics" argument instead.
```

**launch経由だとSPACEでの一時停止が使えません。** 端末が繋がっていないため、`stdin is not a terminal device. Keyboard handling disabled.`と出ます。記録の開始・停止を細かく制御したいなら手打ちのほうが向いています。launchに組み込むのは「起動から終了まで全部録る」用途です。

## 発展

今回のlaunchファイルはremapを直接埋め込んだだけの最小構成でした。実際のプロジェクトでは、`launch.substitutions`や`DeclareLaunchArgument`を使って起動時に引数を渡したり（シミュレータ用と実機用で同じlaunchファイルを使い分けるなど）、`IfCondition`で条件分岐したりします。これは[15_パラメータとlaunchの実践](15_パラメータとlaunchの実践.md)で扱います。

bagについても、今回は記録と単純再生だけでしたが、`ros2 bag play`は特定のトピックだけ再生対象から除外したり、時刻を指定してシークしたりもできます。詳しくは公式ドキュメントを参照してください。

### 記録するかどうかをlaunch引数で切り替える

課題5では記録を無条件に組み込みました。実運用では「デバッグ時だけ録る」「容量が心配なときは切る」という切り替えが欲しくなります。`DeclareLaunchArgument`と`IfCondition`を組み合わせると実現できます。

```python
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration

    DeclareLaunchArgument('record', default_value='true'),
    ExecuteProcess(
        condition=IfCondition(LaunchConfiguration('record')),
        cmd=['ros2', 'bag', 'record', '-a', '-o', 'run'],
        output='screen',
    ),
```

起動時に切り替えます。

```bash
ros2 launch mimic_record_launch.py record:=false
```

**既定値を`true`にしておくのが要点です。** 「録りたいときに`record:=true`を付ける」ようにすると、付け忘れて録れていない事故が起きます。「容量が惜しいときだけ`record:=false`を明示的に付ける」向きにしておけば、忘れたときに安全側（記録される）に倒れます。この考え方は[15_パラメータとlaunchの実践](15_パラメータとlaunchの実践.md)で詳しく扱います。

### ExecuteProcessではなくノードとして起動する方法

`ros2 bag record`はCLIコマンドですが、rosbag2の中身はROSノードです。Jazzyではコンポーネントとして公開されています。

```bash
ros2 component types | grep rosbag
```

```
rosbag2_transport
  rosbag2_transport::Player
  rosbag2_transport::Recorder
```

`rosbag2_transport::Recorder`をコンポーネントコンテナに載せれば、記録処理を他のノードと同じプロセスで動かせます。センサドライバと同じプロセスに載せて、画像を**コピーせずに**記録する（プロセス内通信）といった最適化が可能になります。

ただし設定はすべてパラメータで渡すことになり、`-a`や`--topics`のような手軽さは失われます。**まずは`ExecuteProcess`で十分**です。大容量トピックの記録が性能上の問題になったときに思い出してください。

## おわりに

これで第1部（ROS2の基礎）は完走です。ノード・トピック・サービス・パラメータ・アクション・launch・bagという、ROS 2を使う上で土台になる概念を一通り触ってきました。

ロボコンでの意義を1つだけ強調しておきます。テストランのあとに「さっきの走行で何かおかしかったが、何が原因かわからない」という状況は必ず起きます。そのときにbagが残っていれば、後から`ros2 bag play`で再生しながらオフラインで解析できます。逆にbagを残さずに終わらせたテストランは、不具合が起きても再現する手段がありません。

**そしてこれは「気をつける」で解決しません。** 課題5でやったように、bringup launchに`ExecuteProcess`で記録を組み込んでください。起動すれば必ず録れる状態にしておけば、テストラン当日に思い出す必要がなくなります。

次は第2部、パッケージ開発に入ります。ここまでは既存のノードを組み合わせて動かすだけでしたが、次からは自分でノードを書きます。[10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)でワークスペースの作り方から始めましょう。わからないところがあれば先輩に聞いてください。

### 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `09_launch` — launch を Python / XML / YAML で書く

```bash
./drill run 09
```

課題側からは `./drill read` でこの章に戻ってこられます。

## 資料

- 対応する公式チュートリアル: [Creating a launch file (ROS 2 Jazzy)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Launch/Creating-Launch-Files.html)
- [Recording and playing back data (ROS 2 Jazzy)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Recording-And-Playing-Back-Data/Recording-And-Playing-Back-Data.html)
- 前回: [08_アクション](08_アクション.md)
- 次回（第2部）: [10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)
</content>
