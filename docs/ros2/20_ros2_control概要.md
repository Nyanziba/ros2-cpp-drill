# ROS2講習20: ros2_control概要

## はじめに

この記事を終えると、ros2_controlの全体構成（controller_manager、コントローラ、hardware_interface）を図として説明でき、実機やマイコンにPID計算をどこまで任せるかという設計判断を自分の言葉で議論できるようになります。

前提は[19_URDFの書き方](19_URDFの書き方.md)を読み終えていることです。ros2_controlはURDFの`<ros2_control>`タグを起点に動くため、URDFがない状態では触っても意味がわかりません。

## 講習目標

- controller_manager、コントローラ、hardware_interfaceの役割分担を説明できる
- command interface / state interfaceという概念を理解し、自分のハードに当てはめて考えられる
- 「PID計算をマイコン側でやるか、PC側のコントローラに任せるか」という実際の設計判断について、両方の立場から議論できる
- ros2_control_demosのdiffbot例をmockハードで動かせる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jaliscoがセットアップ済みの環境（[02_環境構築](02_環境構築.md)完了）
- `ros-jazzy-ros2-control`と`ros-jazzy-ros2-controllers`パッケージ（`sudo apt install ros-jazzy-ros2-control ros-jazzy-ros2-controllers`）
- `ros2_control_demos`のクローン先ワークスペース（[10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)完了）
- Gazeboは不要。mockハードウェアで動くデモを使うため、このデモ単体ではシミュレータのインストールを待たなくてよい

### 時間配分の目安

- controller_manager/hardware_interfaceの全体像説明: 15分
- command/state interfaceの図解と実例: 10分
- diffbotデモを動かす演習: 20分
- PIDをどこでやるか問題の議論: 15分

### 口頭試問

**Q1. controller_managerとコントローラ（diff_drive_controllerなど）の役割の違いは何ですか。**

模範解答: controller_managerはURDFの`<ros2_control>`タグを読んで、どのコントローラをどのハードウェアインターフェースに紐づけるかを管理する「司令塔」で、リアルタイムループを一定周期で回してread→update→writeを実行する。コントローラ自身（diff_drive_controllerなど）は、その周期の中で「cmd_velを受け取って左右輪の速度指令に変換する」といった具体的な計算ロジックを持つ。controller_managerはコントローラの入れ替え・起動・停止を担当するが、制御アルゴリズムそのものは持たない。

**Q2. command interfaceとstate interfaceの違いを、自作ハードに当てはめて説明してください。**

模範解答: command interfaceはコントローラからハードウェアへ「こう動いてほしい」と渡す値（例: 関節の目標速度、目標トルク）。state interfaceはハードウェアからコントローラへ「今こうなっている」と返す値（例: エンコーダから読んだ現在位置、現在速度）。自作ハードの場合、hardware_interfaceプラグインのwrite()でcommand interfaceの値をCAN経由でマイコンに送り、read()でマイコンから返ってきたエンコーダ値をstate interfaceに書き込む、という対応になる。

**Q3. PID計算をマイコン側でやる構成と、PC側のros2_controlのコントローラにPIDまで任せる構成、あなたのチームが採っているのはどちらで、なぜそうなっているか説明してください。**

模範解答の一例: マイコン側PIDを採る構成では、マイコンが速度指令（cmd_vel相当）を受け取り、エンコーダのフィードバックを見ながらマイコン内でPID計算まで完結させ、モータドライバに出力する。これは通信レートに依存せず一定周期で制御ループが回るため、CAN通信が多少遅延・欠落してもモータ側の応答性が崩れにくい。PC側にPIDを持たせる構成では、ROS 2側のパラメータでゲインを一元管理でき、シミュレーションのコントローラと実機のコントローラを同じ設定で入れ替えられる利点があるが、PID計算の周期がROS 2の通信レートとcontroller_managerのループ周期に縛られるため、リアルタイム性の要求が厳しい場合は不利になりうる。

## 本文

### 学習内容：controller_managerとhardware_interfaceの全体像

学習内容：ros2_controlが何を分担しているのかを俯瞰する。

準備：[19_URDFの書き方](19_URDFの書き方.md)まで完了していること。

内容：

ros2_controlは「ロボットの関節やモータをROS 2の標準的な仕組みで動かす」ためのフレームワークです。中心にいるのはcontroller_managerというノードで、これが一定周期のリアルタイムループを回して、read（ハードウェアから状態を読む）→update（コントローラの計算を実行する）→write（ハードウェアへ指令を書く）を繰り返します。

構成要素は大きく3つに分けられます。

- controller_manager: コントローラとhardware_interfaceの紐付けを管理し、ループを回す司令塔
- コントローラ: `diff_drive_controller`（差動二輪の速度指令を左右輪指令に変換する）、`joint_trajectory_controller`（アームなどの軌道追従）のように、具体的な制御ロジックを持つプラグイン
- hardware_interface: 実機（またはシミュレータ、mock）との接点。CANやシリアルで実際のモータ・エンコーダとやり取りする部分を自分で実装する

自作ハードとの接点になるのはhardware_interfaceです。ここに`SystemInterface`または`ActuatorInterface`を継承したプラグインを書き、`read()`でエンコーダ値などを取得し、`write()`で指令値をマイコンに送る処理を実装します。逆に言うと、controller_managerやdiff_drive_controllerといった上位の部分は新しく書く必要はなく、hardware_interfaceだけがロボットごとの実装対象になります。

### 学習内容：command interfaceとstate interface

学習内容：controller_managerとhardware_interfaceの間でやり取りされる値の形を理解する。

準備：特になし。

内容：

command interfaceとstate interfaceは、URDFの`<ros2_control>`タグ内で関節ごとに宣言します。

```xml
<ros2_control name="DiffBotSystem" type="system">
  <hardware>
    <plugin>diffbot_base/DiffBotSystemHardware</plugin>
  </hardware>
  <joint name="left_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
  <joint name="right_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
</ros2_control>
```

command interfaceはコントローラからハードウェアへ渡す「こう動いてほしい」の値、state interfaceはハードウェアからコントローラへ返す「今こうなっている」の値です。上の例では左右輪それぞれにvelocityのcommand interfaceと、position/velocityのstate interfaceを持たせています。diff_drive_controllerはcmd_velを受け取ってこのvelocity command interfaceに値を書き込み、hardware_interfaceのwrite()がその値を実際のモータドライバに送ります。

interfaceの型（position/velocity/effort）はコントローラが要求するものと一致していなければ組み合わせられません。`diff_drive_controller`はvelocity command interfaceを要求するので、hardware側でeffort（トルク）しか用意していないと繋がりません。この対応関係を確認せずに動かないと騒ぐのはよくある詰まりポイントです。

### 学習内容：diffbotデモをmockハードで動かす

学習内容：ros2_control_demosのdiffbot例をcloneして、実機なし・Gazeboなしで動作を確認する。

準備：`ros-jazzy-ros2-control`、`ros-jazzy-ros2-controllers`がインストール済みであること。

内容：

ワークスペースのsrc以下にデモをcloneします。

```bash
cd ~/ros2_ws/src
git clone -b jazzy https://github.com/ros-controls/ros2_control_demos.git
cd ~/ros2_ws
colcon build --packages-up-to ros2_control_demo_example_2
source install/setup.bash
```

diffbot例（`ros2_control_demo_example_2`）はmockハードウェアで動作するため、実機もGazeboも不要です。URDFの`<ros2_control>`タグの中でmockハードウェアプラグインを指定しているだけで、controller_managerやdiff_drive_controllerの挙動そのものはそのまま確認できます。

```bash
ros2 launch ros2_control_demo_example_2 diffbot.launch.py
```

起動したら別のターミナルで、動いているコントローラを確認します。

```bash
ros2 control list_controllers
```

`diff_drive_controller`が`active`になっていれば、cmd_velを送って動作を確認できます。

```bash
ros2 topic pub --rate 10 /cmd_vel geometry_msgs/msg/TwistStamped \
  "{twist: {linear: {x: 0.3}, angular: {z: 0.1}}}"
```

トピック名に注意してください。`diff_drive_controller`のデフォルトは`~/cmd_vel`（つまり`/diffbot_base_controller/cmd_vel`）ですが、`ros2_control_demos`の`diffbot.launch.py`はspawnerに`--controller-ros-args "-r ~/cmd_vel:=/cmd_vel"`を渡してリマップしているため、実際に購読されているのは`/cmd_vel`です。`/diffbot_base_controller/cmd_vel`に投げても誰も受け取りません。困ったら`ros2 topic info -v /cmd_vel`で購読者を確かめるのが確実です。

mockハードウェアなので実際に何かが動くわけではありませんが、`/dynamic_joint_states`や`/joint_states`をechoすると、command interfaceに書き込まれた値がそのままstate interfaceに反映されて返ってくる様子が見えます。この「コントローラが計算した値がhardware_interfaceに渡り、また戻ってくる」というループの実感を掴むのがこの演習の目的です。

**練習問題**: `ros2 control list_hardware_interfaces`を実行し、`left_wheel_joint`と`right_wheel_joint`にそれぞれどんなcommand interfaceとstate interfaceが定義されているか確認してください。

<details><summary>解答</summary>

`ros2 control list_hardware_interfaces`を実行すると、command interfacesにvelocity、state interfacesにposition/velocityが両輪分リストされます。これはdiffbotのURDF（`ros2_control_demo_example_2`のdescriptionパッケージ内）の`<ros2_control>`タグでの宣言と一致します。実機を用意する代わりに、まずこのURDF側の宣言を読むと、どんな値が流れる想定なのかが先にわかります。

</details>

### 学習内容：PID計算をどこでやるか問題

学習内容：マイコン側PIDとPC側ros2_controlコントローラのPID、2つの構成を比較する。

準備：特になし。

内容：

マイコン側PIDの構成では、PC側はcmd_vel相当の速度指令をCAN経由でマイコンに送るだけで、実際のPID計算（目標速度とエンコーダ値の誤差をどう縮めるか）はマイコン側で行っています。ros2_controlを導入する場合、この計算をどこに置くかで構成が変わります。

マイコン側PIDは、モータドライバに近い場所で一定周期の制御ループが回るため、CAN通信が多少乱れても制御ループ自体は止まりません。ゲイン調整はマイコンのファームウェアを書き換えるかシリアル経由でパラメータを送る仕組みを別に作る必要があります。

PC側のros2_controlコントローラ（例えば`diff_drive_controller`の先に速度制御を担うコントローラを挟む、あるいはPID用のコントローラプラグインを使う）にPID計算まで任せる構成では、ゲインはROS 2パラメータとして`ros2 param set`やYAMLで一元管理でき、実機用コントローラとシミュレーション用コントローラを同じインターフェースで入れ替えられます。デバッグ時に`ros2 control list_controllers`やrqtでゲインを覗けるのも利点です。ただしこの構成では、controller_managerのループ周期とマイコンとの通信レート（CANであれば送受信の往復にかかる時間）がそのまま制御周期の下限になります。センサ値の読み取りから指令の送信までがROS 2の通信を経由する分、マイコン内で完結する構成より遅延が増え、通信が詰まったときの影響も大きくなります。

どちらが正しいという話ではなく、要求される制御周期とマイコン・PC間の通信の余裕次第です。低速で余裕のある機構ならPC側に寄せてパラメータ管理の楽さを取る選択もあり得ますし、応答性が要求される駆動系では従来通りマイコン側PIDを維持したまま、ros2_controlはあくまで上位からの指令経路の統一に使うという構成も考えられます。

## 発展

ros2_controlを自分の実機で採用するかどうかは、この記事だけでは結論を出しません。判断材料になる軸を挙げておきます。

- URDFが実機の全リンク・全関節分すでに整備されているか（整備コストがそのまま導入コストになる）
- 制御周期の要求値と、CAN/シリアルの実測レイテンシの比較
- シミュレーション（Gazebo/MuJoCo）との入れ替えをどこまで重視するか
- マイコン側のファームウェアをどこまで薄くしたいか（薄くするほどPC側の責務とリアルタイム性要求が増える）
- チーム内でROS 2パラメータでのゲイン管理の経験がどれだけあるか


## おわりに

ros2_controlはURDFがあることが前提の仕組みです。前提を整えずに触ると「なぜ動かないのか」で詰まるので、順番を守って[19_URDFの書き方](19_URDFの書き方.md)から進んでください。わからなければ先輩に聞きましょう。

次は[21_センサ統合](21_センサ統合.md)で、LiDARやIMUをROS 2側に取り込む方法を扱います。

## 資料

- [ros2_control公式（control.ros.org）](https://control.ros.org/jazzy/index.html)
- [ROS 2 Documentation: Jazzy — ros2_control関連ページ](https://docs.ros.org/en/jazzy/p/ros2_control/)
- [ros2_control_demos（GitHub）](https://github.com/ros-controls/ros2_control_demos)
- [19_URDFの書き方](19_URDFの書き方.md)
- [21_センサ統合](21_センサ統合.md)
