# ROS2講習05b: DDSとdiscoveryとROS_DOMAIN_ID

## はじめに

この記事を終えると、「なぜ隣の人のノードが自分の`ros2 node list`に出てくるのか」「なぜ`ROS_DOMAIN_ID`を設定すると出てこなくなるのか」を仕組みから説明できるようになります。

[05_トピック](05_トピック.md)の発展節で「深掘りは別記事に譲ります」と書いた、その別記事です。

**この記事は複数人が同じネットワークで実際に起きるトラブルへの対処記事です。** 複数人が同じネットワークにつないでROS2を動かすと、次のようなことが起きます。

- 自分は`turtlesim`しか起動していないのに`ros2 node list`に見知らぬノードが並ぶ
- 自分の`cmd_vel`が、隣の人のロボットを動かしてしまう
- テストが自分の環境では通るのに、複数人が同じネットワークで作業していると落ちる

どれも「バグ」ではなく、DDSのdiscoveryが設計どおりに動いた結果です。仕組みを知っていれば1コマンドで防げます。

前提は[05_トピック](05_トピック.md)まで読み終えていることです。

## 講習目標

- ROS2の通信がDDS/rmw/rclの層構造の上に成り立っていることを説明できる
- discoveryがどうやってノードを見つけているかを説明できる
- `ROS_DOMAIN_ID`と`ROS_AUTOMATIC_DISCOVERY_RANGE`の違いを理解し、状況に応じて使い分けられる
- 「他人のノードが見える」トラブルを自力で切り分け、解消できる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jaliscoがセットアップ済みの環境（[02_環境構築](02_環境構築.md)完了）
- `ros-jazzy-demo-nodes-cpp`パッケージ（未インストールなら`sudo apt install ros-jazzy-demo-nodes-cpp`）
- ターミナルを2枚以上開ける画面
- `ss`コマンド（`iproute2`パッケージ。Ubuntu 24.04には標準で入っています）
- **できれば2台以上のPC**。同じネットワークにつないだ2台があると「他人のノードが見える」を実演できます。1台でも記事の課題はすべて実施できます

### 時間配分の目安

- 層構造の説明（DDS/rmw/rcl）: 10分
- discoveryの実験（ドメイン分離）: 20分
- ポート番号の確認: 10分
- `ROS_AUTOMATIC_DISCOVERY_RANGE`の実験: 15分
- トラブルシュート演習と口頭試問: 15分

### 口頭試問

**Q1. `ros2 node list`が他人のノードを表示してしまうのは、なぜですか。バグですか。**

模範解答: バグではなく、DDSのdiscoveryが仕様どおりに動いた結果。DDSは起動時にネットワークへマルチキャストで「自分はここにいる」と広告し、他の参加者からの広告を受け取る。既定の`ROS_AUTOMATIC_DISCOVERY_RANGE`は`SUBNET`で、同じサブネット全体に広告が届く。そのため同じLANにいる別の人のノードも互いに見えてしまう。ROS2は「1台のロボットの中の複数ノードが自動で繋がる」ことを想定した既定値になっているため、複数人が1つのLANで別々のロボットを開発する状況では明示的に分離する必要がある。

**Q2. `ROS_DOMAIN_ID`と`ROS_AUTOMATIC_DISCOVERY_RANGE`は、どちらも「他人と混ざらないようにする」ための設定です。違いを説明してください。**

模範解答: `ROS_DOMAIN_ID`は「チャンネルを変える」設定で、使うUDPポート番号そのものが変わる（`7400 + 250 × ドメインID`が基準）。ドメインが違えば別のポートを使うので、同じネットワークにいても互いに見えない。`ROS_AUTOMATIC_DISCOVERY_RANGE`は「広告の届く範囲を変える」設定で、`LOCALHOST`にすると自分のPCの外へ広告を出さない。前者は「同じ部屋で違うチャンネルの無線を使う」、後者は「そもそも部屋の外に電波を出さない」に相当する。同じPCの中で複数人が作業することはないので、実用上は`LOCALHOST`のほうが確実。両方併用するとさらに安全。

**Q3. `ROS_DOMAIN_ID`に指定していい数値の範囲と、その理由を説明してください。**

模範解答: 仕様上の最大は232。`7400 + 250 × 232 + 11 = 65411`で、これを超えるとUDPポート番号の上限65535を超えてしまうため。ただし実用上は**0〜101**に収めるのが安全。Linuxのephemeralポート範囲（`/proc/sys/net/ipv4/ip_local_port_range`、多くの環境で32768〜60999）と衝突しないのは、ドメイン101の`7400 + 250 × 101 = 32650`までだから。102以上を使うと、たまたま他のアプリが同じポートを先に取っていて起動に失敗する、という再現しにくい不具合が起きうる。

**Q4. 部室で「テストが自分の環境では通るのに、みんなが作業していると落ちる」という報告を受けました。どう切り分けますか。**

模範解答: まず`ros2 node list`と`ros2 topic list`で、自分が起動していないノード・トピックが見えていないかを確認する。見えていればDDSの混信を疑い、`ROS_DOMAIN_ID`を他人と重ならない値にするか、`ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST`を設定して再実行する。それで通るなら混信が原因。テストが同名のトピック（`/chatter`や`/cmd_vel`のような一般的な名前）を使っている場合に起きやすい。恒久対策としてはテストランナー側で環境変数を固定する（`ros2-drill`の`drill`スクリプトがこの方式）。

## 本文

### 学習内容：ROS2の下には何があるか

学習内容：DDS / rmw / rcl / rclcpp の層構造を理解する。

準備：特になし。

内容：

[05_トピック](05_トピック.md)で「トピックの裏側でDDSが動いている」と書きました。もう少し正確に言うと、ROS2は4層になっています。

```
+----------------------------------------------------+
|  あなたのコード（MinimalPublisher など）              |
+----------------------------------------------------+
|  rclcpp / rclpy  … 言語ごとのAPI                     |
+----------------------------------------------------+
|  rcl             … 言語非依存のC API                 |
+----------------------------------------------------+
|  rmw             … DDSを抽象化したCインタフェース      |
+----------------------------------------------------+
|  DDS実装（Fast DDS / Cyclone DDS / ...）              |
+----------------------------------------------------+
```

**トピック名の解決やQoSの互換判定は`rcl`と`rmw`が担当し、実際にパケットをネットワークへ流すのはDDS実装**です。ROS2自身は通信プロトコルを持っていません。DDSという既存の産業用ミドルウェアを借りてきて、その上にROSの概念（ノード、トピック、サービス）を被せた構造です。

自分の環境でどのDDS実装が使われているかは`ros2 doctor`で確認できます。

```bash
ros2 doctor --report
```

`RMW MIDDLEWARE`の節に出ます。

```
   RMW MIDDLEWARE
middleware name    : rmw_fastrtps_cpp
```

**Jazzyの既定はFast DDS（`rmw_fastrtps_cpp`）です。** eProsima社の実装で、apt でROS2を入れると一緒に入ってきます。

別の実装に切り替えることもできます。

```bash
sudo apt install ros-jazzy-rmw-cyclonedds-cpp
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
```

ただし**この講習では切り替えません。** 既定のFast DDSで進めます。実装を変えるとdiscoveryの細かい挙動や設定ファイルの書式まで変わるので、明確な理由（特定のバグを避けたい、など）がないうちは触らないほうがよいです。

なお`ROS_DOMAIN_ID`や`ROS_AUTOMATIC_DISCOVERY_RANGE`は`rcl`層が読む環境変数なので、**DDS実装を変えても同じように効きます。** これらが「ROS2の設定」であって「Fast DDSの設定」ではないという区別は、後々設定を調べるときに効いてきます。

### 学習内容：discoveryが何をしているか

学習内容：ノードが互いを見つける仕組みを理解する。

準備：ターミナル2枚。

内容：

`ros2 run demo_nodes_cpp talker`を起動して、別のターミナルで`ros2 node list`を打つと`/talker`が出てきます。このとき、あなたは**IPアドレスもポート番号も一度も指定していません。**

これを実現しているのがdiscoveryです。DDSの参加者（participant）は起動すると次のことをします。

1. 「自分はここにいる。こういうトピックをpublishする」という広告をネットワークへマルチキャストで投げる
2. 他の参加者からの同じ広告を待ち受ける
3. 広告を受け取ったら、相手のトピック名とQoSを見て、自分と噛み合うものがあれば接続する

**「噛み合うものがあれば」の判定にQoSが使われます。** だから[05_トピック](05_トピック.md)で扱ったQoSの不一致が「繋がらない」という形で表れるのです。discoveryは成功していて、接続判定で落ちている、という状態です。

やってみましょう。ターミナル1で talker を起動します。

```bash
ros2 run demo_nodes_cpp talker
```

ターミナル2で確認します。

```bash
ros2 node list
ros2 topic list
```

```
/talker
```

```
/chatter
/parameter_events
/rosout
```

`/parameter_events`と`/rosout`は、あなたが作っていないのに出てきます。これはノードを作ると自動で付いてくるトピックで、パラメータ変更の通知とログの配信に使われます。

> コラム: `ros2 node list`の結果は`ros2 daemon`というバックグラウンドプロセスがキャッシュしています。`ros2 daemon status`で状態が見られます。ノードを止めたのにリストに残り続ける、というときはこのキャッシュを疑ってください。`ros2 daemon stop`で止めれば次のコマンドで作り直されます。この記事の実験でも、環境変数を変えたのに結果が変わらないときはまずdaemonを止めてください。

### 学習内容：ROS_DOMAIN_IDでチャンネルを分ける

学習内容：ドメインを変えるとノードが互いに見えなくなることを確認する。

準備：ターミナル2枚。前の課題の talker は止めておきます。

内容：

ターミナル1でドメイン42を指定して talker を起動します。

```bash
export ROS_DOMAIN_ID=42
ros2 run demo_nodes_cpp talker
```

ターミナル2で、**同じドメイン**から見ます。

```bash
ROS_DOMAIN_ID=42 ros2 node list
```

```
/talker
```

見えます。次に**違うドメイン**から見ます。

```bash
ROS_DOMAIN_ID=43 ros2 node list
```

何も出ません。talker は動いているのに、ドメイン43からは存在しないのと同じです。

**これが部室での混信対策の基本形です。** 各自が別のドメインを使えば、同じネットワークにいても互いに干渉しません。

#### 何が起きているのか — ポート番号を見る

「チャンネルが違う」の実体はUDPポート番号です。実際に見てみましょう。talker を動かしたまま、そのプロセスが開いているUDPポートを調べます。

```bash
pid=$(pgrep -x talker)
ss -ulnp | grep "$pid"
```

ドメイン42で動かしたときの実際の出力です（一部を抜粋）。

```
UNCONN 0  0  0.0.0.0:17900  0.0.0.0:*  users:(("talker",pid=155463,fd=26))
UNCONN 0  0  0.0.0.0:17910  0.0.0.0:*  users:(("talker",pid=155463,fd=25))
UNCONN 0  0  0.0.0.0:17911  0.0.0.0:*  users:(("talker",pid=155463,fd=28))
```

`17900`、`17910`、`17911`。この数字はRTPS（DDSのワイヤプロトコル）の仕様で決まっています。

```
基準ポート        = 7400 + 250 × ドメインID
discovery用       = 基準ポート
discovery用(個別) = 基準ポート + 10 + 2 × 参加者番号
データ用(個別)    = 基準ポート + 11 + 2 × 参加者番号
```

ドメイン42で計算してみます。

```
7400 + 250 × 42 = 17900   ← discovery用（マルチキャスト）
17900 + 10 + 0  = 17910   ← discovery用（このプロセス個別）
17900 + 11 + 0  = 17911   ← データ用（このプロセス個別）
```

**観測した3つの数字と完全に一致します。** ドメインIDを変えるとこの基準ポートが250ずつずれるので、別のドメインのパケットは物理的に届きません。「見えない」のではなく「別のポートを見ている」のです。

#### 使ってよい数値の範囲

計算式が分かると、指定できる上限も導けます。

| ドメインID | 基準ポート | データ用ポート | 判定 |
|---|---|---|---|
| 0 | 7400 | 7411 | OK |
| 42 | 17900 | 17911 | OK |
| 101 | 32650 | 32661 | OK（実用上の上限） |
| 232 | 65400 | 65411 | OK（仕様上の上限） |
| 233 | 65650 | 65661 | **UDPポートの上限65535を超える** |

**仕様上の最大は232**です。では101を「実用上の上限」としているのはなぜか。Linuxがアプリに自動割り当てするポート範囲（ephemeralポート）と衝突するからです。

```bash
cat /proc/sys/net/ipv4/ip_local_port_range
```

```
32768	60999
```

**32768以上は、他のアプリが勝手に使う可能性のある領域**です。ドメイン101の基準ポートは32650で、ぎりぎりこの手前に収まります。102以上を選ぶと、たまたま別のプロセスが同じポートを先に取っていてノードが起動できない、という再現しにくい不具合を踏む可能性があります。

**実用上は0〜101の範囲から選んでください。** 迷ったらユーザごとに一意な値（ユーザID、学籍番号など）を基に決めると衝突しません。

### 学習内容：ROS_AUTOMATIC_DISCOVERY_RANGEで範囲を絞る

学習内容：広告の届く範囲を制御する。

準備：ターミナル2枚。

内容：

`ROS_DOMAIN_ID`は「チャンネルを変える」設定でした。もう1つ、「そもそも外に広告を出さない」という設定があります。

`ROS_AUTOMATIC_DISCOVERY_RANGE`に指定できる値は4つです。ROS2のヘッダ（`/opt/ros/jazzy/include/rmw/rmw/discovery_options.h`）に定義されています。

| 値 | 意味 |
|---|---|
| `OFF` | 自動discoveryをしない |
| `LOCALHOST` | 自分のPCの中だけ |
| `SUBNET` | 同じサブネット全体（**既定値**） |
| `SYSTEM_DEFAULT` | DDS実装側の設定に従う |

**既定が`SUBNET`であることが、部室のトラブルの原因です。** 何も設定しなければ、あなたのノードは同じLAN全体に「ここにいます」と広告し続けます。

実験してみましょう。ターミナル1で`LOCALHOST`にして talker を起動します。

```bash
ROS_DOMAIN_ID=45 ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST ros2 run demo_nodes_cpp talker
```

ターミナル2から、同じ設定で見ます。

```bash
ROS_DOMAIN_ID=45 ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST ros2 node list
```

```
/talker
```

**同じPCの中なので見えます。** `LOCALHOST`は「自分のPCの中は繋がるが、外には出ない」設定です。1台のPCで開発している限り、何も不便はありません。

2台のPCがあるなら、もう1台から`ROS_DOMAIN_ID=45 ros2 node list`を打ってみてください。`SUBNET`（既定）なら見えたはずのノードが、`LOCALHOST`では見えません。これが狙った効果です。

#### OFFにすると何も見えなくなる

`OFF`はdiscoveryそのものを止めます。試すとROS2が親切に警告してくれます。

```bash
ROS_DOMAIN_ID=44 ROS_AUTOMATIC_DISCOVERY_RANGE=OFF ros2 node list
```

```
Warning: ROS_AUTOMATIC_DISCOVERY_RANGE=OFF with no ROS_STATIC_PEERS configured.
No discovery mechanism is available. Results will be empty.
Either:
  - Set ROS_STATIC_PEERS to specify peers explicitly, or
  - Change ROS_AUTOMATIC_DISCOVERY_RANGE to LOCALHOST or SUBNET
```

**同じPCの中の talker すら見えません。** `OFF`は「通信相手を自分で全部列挙する」という運用のための設定で、そのときに使うのが`ROS_STATIC_PEERS`です。

```bash
export ROS_AUTOMATIC_DISCOVERY_RANGE=OFF
export ROS_STATIC_PEERS="192.168.1.10;192.168.1.11"
```

`;`で区切って複数指定できます。マルチキャストが通らないネットワーク（一部の無線APやクラウド環境）で使う手段です。**通常のロボット開発では使いません。** 名前だけ知っておいてください。

#### どちらを使うべきか

| 状況 | 推奨 |
|---|---|
| 1台のPCで開発・テストする | `ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST` |
| 部室で複数人が別々のロボットを開発する | 各自が別の`ROS_DOMAIN_ID`（0〜101） |
| 1台のロボットを複数PCで分担して動かす | 同じ`ROS_DOMAIN_ID` + `SUBNET`（既定のまま） |
| マルチキャストが通らないネットワーク | `OFF` + `ROS_STATIC_PEERS` |

**迷ったら`LOCALHOST`にしてください。** 複数PCで分担する構成は実機運用の段階で初めて必要になります。それまでは`LOCALHOST`で困ることはありません。

なお、ROS2のヘッダにはこう書かれています。

> It is intended that the default will be LOCALHOST in future versions of ROS.

**将来のROS2では既定が`LOCALHOST`になる予定**だと明記されています。今`LOCALHOST`を設定しておくのは、先取りしているだけで特殊なことではありません。

### 学習内容：テスト環境での実例

学習内容：実際のコードでどう設定されているかを読む。

準備：特になし（コードを読むだけ）。

内容：

C++/ROS2練習教材の`ros2-drill`のテストランナーは、この2つの環境変数を両方設定しています。`drill`スクリプトの`ros_env()`関数です。

```python
def ros_env():
    env = os.environ.copy()
    # 同じ LAN の他の受講者と DDS が混信しないよう閉じておく。
    env.setdefault("ROS_AUTOMATIC_DISCOVERY_RANGE", "LOCALHOST")
    env.setdefault("ROS_DOMAIN_ID", str(1 + os.getuid() % 99))
```

出典: `ros2-drill/drill`（`ros_env()`）

2行で何をしているか読み解いてください。

**1行目**は広告を自分のPCの外に出さない設定です。講習で複数人が同じネットワークにいても、互いのテストが干渉しません。

**2行目**はドメインIDをUNIXのユーザIDから決めています。`1 + uid % 99`なので**1〜99の範囲**に収まります。前の課題で見た「0〜101が安全」の中に入っています。

```bash
id -u
```

このマシンでは`1000`だったので、`1 + 1000 % 99 = 11`。ドメイン11が使われます。

**なぜuidから決めるのか。** 同じPCで別のユーザが同時に作業した場合でも衝突しないためです。そして「人が手で設定しなくても自動で分かれる」ことが重要です。受講者に「各自ドメインIDを設定してください」と指示すると、必ず誰かが忘れます。

**`setdefault`を使っている点にも注目してください。** すでに環境変数が設定されていれば、そちらを尊重します。「既定は安全側、必要なら上書き可能」という設計です。

このテストは同じプロセス内に受講者のノードと検証用ノードを載せて動かすので、そもそもネットワークに出る必要がありません。だから`LOCALHOST`で何も失いません。**「必要のない通信範囲は閉じる」のが原則**です。

## 発展

### discovery serverという選択肢

ここまでのdiscoveryは「全員が全員に広告する」方式です。ノードが増えると広告のトラフィックが参加者数の2乗で増えるため、数十ノード規模になると問題になります。

Fast DDSには**discovery server**という仕組みがあり、1台のサーバに問い合わせる形に変えられます。広告のトラフィックが線形に収まります。

ほとんどのロボットシステムは現時点でこの規模に達していないので使っていません。`ros2 node list`が返ってくるまで数秒かかるようになったら検討する、という段階の話です。名前だけ覚えておいてください。

### QoSの不一致とdiscoveryの失敗は別物

「トピックが繋がらない」とき、原因は2種類あります。切り分けの手順を持っておくと早いです。

| 症状 | 原因 | 確認方法 |
|---|---|---|
| `ros2 node list`に相手が出てこない | **discoveryの失敗** | ドメインID、discovery range、ネットワーク |
| 相手は見えるが`ros2 topic echo`に何も出ない | QoS不一致、またはpublish側の実装 | `ros2 topic info -v` |

**まず`ros2 node list`で相手が見えるかを確認してください。** 見えていればdiscoveryは成功しているので、ドメインIDをいくら変えても解決しません。QoS（[05_トピック](05_トピック.md)）か、publisherの実装（[11_C++でpub_subを書く](11_C++でpub_subを書く.md)）を疑う場面です。

C++で書いていて「publishしているのに何も出ない」場合、**`create_publisher()`の戻り値をメンバ変数に保持し忘れている**というのが最も多い原因です。rclcppは作ったエンティティを`weak_ptr`でしか持たないので、呼び出し側が握り続けなければ消えます。エラーも警告も出ません。[C++編 6章 スマートポインタ](../cpp/06_スマートポインタ.md)で仕組みから扱っています。

### マルチキャストが通るか確認する

discoveryはマルチキャストに依存しています。ネットワーク側でマルチキャストが遮断されていると、設定が正しくてもノードが見つかりません。

ROS2にはこれを直接確かめるコマンドがあります。1台で受信して、もう1台から送ります。

```bash
# 受信側（先に起動して待たせる）
ros2 multicast receive
```

```bash
# 送信側
ros2 multicast send
```

送信側はこう出て終了します。

```
Sending one UDP multicast datagram...
```

受信側にこう出れば、マルチキャストは通っています（同一マシンでの実測）。

```
Waiting for UDP multicast datagram...
Received from 10.28.0.217:37857: 'Hello World!'
```

受信側が`Waiting for ...`のまま止まっていれば通っていません。ネットワーク（無線APの設定、ファイアウォール、コンテナのネットワークモード）を疑う段階です。

**「ROS2の設定を疑う前に、ネットワークが通っているかを確かめる」** という切り分けの道具として覚えておいてください。ROS2側をいくら調べても解決しない類の問題があります。

### Dockerを使う場合

[02_環境構築](02_環境構築.md)でDockerを選んだ場合、既定のbridgeネットワークではコンテナ内のノードとホストのノードが互いに見えません。マルチキャストがコンテナのネットワーク境界を越えないためです。

`--network host`でホストのネットワークを共有すれば見えるようになりますが、その場合はホスト側のノードと完全に混ざるので、`ROS_DOMAIN_ID`での分離がより重要になります。


## おわりに

この記事で扱ったのは、突き詰めると2つの環境変数です。

```bash
export ROS_DOMAIN_ID=11                        # チャンネルを変える（0〜101）
export ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST # 外に広告を出さない
```

**`~/.bashrc`に書いておくことを勧めます。** 部室で作業するたびに設定するのは忘れます。忘れたときに起きるのは「自分のテストが落ちる」だけでは済まず、「隣の人のロボットを動かしてしまう」ところまであり得ます。

そして「なぜこれで直るのか」を説明できるようにしておいてください。おまじないとして覚えると、少し違う症状が出たときに応用が効きません。ポート番号の計算式まで戻れれば、`ros2 node list`が空になる原因を自分で辿れます。

次は[06_サービス](06_サービス.md)に戻って、トピックとは対になるリクエスト/レスポンス型の通信に進んでください。

## 資料

- [ROS 2 Documentation: Jazzy — The ROS_DOMAIN_ID](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Domain-ID.html)
- [ROS 2 Documentation: Jazzy — Discovery](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Discovery.html)
- [ROS 2 Documentation: Jazzy — About Quality of Service settings](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html)
- [ROS 2 Documentation: Jazzy — About internal ROS 2 interfaces (rmw)](https://docs.ros.org/en/jazzy/Concepts/Advanced/About-Internal-Interfaces.html)
- [eProsima Fast DDS — Discovery server](https://fast-dds.docs.eprosima.com/en/latest/fastdds/discovery/discovery_server.html)
- `/opt/ros/jazzy/include/rmw/rmw/discovery_options.h` — `ROS_AUTOMATIC_DISCOVERY_RANGE`の4つの値の定義
- `/opt/ros/jazzy/include/rcl/rcl/discovery_options.h` — 既定値が`SUBNET`であること、将来`LOCALHOST`にする意向、`ROS_STATIC_PEERS`の区切り文字
- [05_トピック](05_トピック.md) — この記事の前提。QoSの基本
- [02_環境構築](02_環境構築.md) — Docker構成を選んだ場合の前提
