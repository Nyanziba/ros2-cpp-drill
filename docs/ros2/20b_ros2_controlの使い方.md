# ROS2講習20b: ros2_controlの使い方（差動二輪の足回りを例に）

## はじめに

この記事を終えると、差動二輪ロボットのワークスペース例を読んで、自作hardware_interfaceの書き方・コントローラの設定・起動確認まで一通り追えるようになります。

前提は[20_ros2_control概要](20_ros2_control概要.md)です。controller_manager/コントローラ/hardware_interfaceの3層構造は前提として説明を省略するので、わからなければ先に20を読んでください。この記事は概念編の続きの実践編で、実在するコードを読みながらros2_controlを使う側の視点をつかむのが目的です。

## 講習目標

- ロボットプロジェクトのワークスペース構成と、ros2_control完全移行という設計判断を説明できる
- `ExampleChipmdHardware`を例に、SystemInterfaceのライフサイクル（on_init→on_configure→on_activate→read/writeループ）とURDFの`<ros2_control>`タグの対応を読める
- `controllers.yaml`の内容とspawnerの役割を理解し、起動順序を説明できる
- `ros2 control list_controllers`/`list_hardware_interfaces`で状態を確認する手順を実行できる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jaliscoがセットアップ済みの環境（[02_環境構築](02_環境構築.md)完了）
- ロボットプロジェクトのワークスペースをclone済み（実機がなくても`use_mock_hardware:=true`で動く）
- `ros-jazzy-ros2-control`、`ros-jazzy-ros2-controllers`（pixi環境なら`pixi.toml`経由で入るはずなので、入っていなければ先に確認する）
- 実機のCAN/シリアル接続は不要。この講習はmockハードウェアで完結させる

### 時間配分の目安

- ワークスペース構成とADR 0004の要約説明: 10分
- `robot_chipmd_hardware.cpp`を読みながらライフサイクル解説: 20分
- `controllers.yaml`とlaunchの起動順序解説: 15分
- mock起動して`list_controllers`/`list_hardware_interfaces`を叩く演習: 15分
- 口頭試問: 10分

### 口頭試問

**Q1. readとwriteはそれぞれ何をするか、`ExampleChipmdHardware`を例に説明してください。**

模範解答: readはハードウェアからの最新フィードバックをstate interfaceに反映する処理。`robot_chipmd_hardware.cpp`の`read()`は`rx_buffer_`（`/motor/state`トピックから受け取ったエンコーダ値のRealtimeBuffer）を読み、タイムアウトしていなければ`joint.state_position`/`joint.state_velocity`を更新する。writeはcommand interfaceに書き込まれた値をハードウェアへ送る処理。`write()`は`joint.command_voltage`を`tx_buffer_`に詰め、別スレッドの1kHzタイマ（`publish_latest_command`）がそれを`/board/chipmd/target`にpublishしてCAN経由でマイコンに渡す。read/writeそのものはROS通信を直接叩かず、事前に用意したリアルタイムセーフなバッファを介するだけという点が重要。

**Q2. spawnerは何をspawnするのか、`hardware_spawner`と`spawner`の違いも含めて説明してください。**

模範解答: `controller_manager`パッケージが提供する2種類のNodeで、`hardware_spawner`はハードウェアコンポーネント（例: `ExampleChipmdWheelSystem`）を`--activate`で configure→activate まで進める。`spawner`はコントローラ（`joint_state_broadcaster`や`mobility_controller`など）をcontroller_managerにロードしてactive化する。`_subsystem_bringup.py`ではhardware_spawnerが終了(`OnProcessExit`)してから controller spawnerを起動しており、これは「コントローラは対応するハードウェアがactiveになってからでないとinterfaceを掴めない」という順序制約のため。

**Q3. `base_controllers.yaml`で全ハードウェアコンポーネントの初期状態が`unconfigured`になっているのはなぜですか。**

模範解答: `hardware_components_initial_state`が`unconfigured`だと、base.launch.pyだけを起動した時点では誰もデバイスを開かない。各サブシステム(`mobility`/`hand`/`lift`)のlaunchが自分の使うコンポーネントだけを`hardware_spawner --activate`で明示的に立ち上げる設計になっているため、ホイールだけをベンチテストしたいときにハンドのシリアルポート(`/dev/feetech`)まで開いてしまう、といった不要な副作用を避けられる。

## 本文

### ワークスペースの全体像

この例のワークスペースは、領域ごとにパッケージを分けています。

```text
src/
├── bringup/    robot_bringup（launch・controllers.yaml集約）
├── comm/       robot_comm_board_router, robot_comm_can_gateway
├── control/    robot_mobility_controller, robot_lift_position_controller, ...
├── description/ robot_description（URDF/xacro）
├── hardware/   robot_hardware_chipmd, _solenoid, _sts3215
├── msgs/       robot_ros_msgs
├── operation/  robot_operation_teleop
├── perception/ robot_perception_lidar
└── utils/      robot_external_protocol_check
```

`src/<領域>/<package>`という配置で、launchと共通設定は`robot_bringup`に集約するというのがこのリポのルールです（`README.md`より）。hardware/にあるパッケージが自作hardware_interfaceの実装、control/にあるパッケージが自作コントローラの実装で、この2つがros2_controlのプラグインとして差し込まれる部分になります。

このリポは`ADR 0004: ros2_control 完全移行アーキテクチャへの更新`という設計判断を経ています。当初はwheel/liftだけの部分移行を計画していたものを、「アーキテクチャの統一性と保守性を高めるため」全ハードウェアコンポーネントをros2_controlで統合する完全移行に切り替えたと書かれています。以前のPoC実装ではカスケード制御やvelocity commandの直接出力といった複雑な経路があったものの、本番アーキテクチャではそれらを排除し、Wheel経路は

```text
/cmd_vel → ExampleMobilityController → (wheel velocity references) → Wheel PID Controllers → (voltage command) → ExampleChipmdHardware
```

という単一経路に統一しています（`docs/adr/0004-ros2-control-migration.md`）。直接velocity commandをhardware_interfaceに渡す経路を廃止し、PIDコントローラを必ず経由させるという判断です。1つの記事にPID計算を「マイコン側でやるかPC側でやるか」の議論がありますが（[20_ros2_control概要](20_ros2_control概要.md)参照）、このリポはPC側のコントローラでPID（`wheel_pid_controller`）を挟む構成を選んでいます。

### 自作hardware_interfaceを読む: ExampleChipmdHardware

`src/hardware/robot_hardware_chipmd/`が、ChipMDモータドライバ基板（ホイール4輪+リフト1軸を管理）向けの`hardware_interface::SystemInterface`実装です。ヘッダ（`include/robot_hardware_chipmd/robot_chipmd_hardware.hpp`）を見ると、ライフサイクルのメソッドが一通り並んでいます。

```cpp
hardware_interface::CallbackReturn on_init(
  const hardware_interface::HardwareComponentInterfaceParams& params) override;
std::vector<hardware_interface::StateInterface> export_state_interfaces() override;
std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;
hardware_interface::CallbackReturn on_activate(const rclcpp_lifecycle::State&) override;
hardware_interface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State&) override;
hardware_interface::CallbackReturn on_error(const rclcpp_lifecycle::State&) override;
hardware_interface::CallbackReturn on_shutdown(const rclcpp_lifecycle::State&) override;
hardware_interface::return_type read(const rclcpp::Time&, const rclcpp::Duration&) override;
hardware_interface::return_type write(const rclcpp::Time&, const rclcpp::Duration&) override;
```

（出典: `src/hardware/robot_hardware_chipmd/include/robot_hardware_chipmd/robot_chipmd_hardware.hpp`）

`on_configure`が別立てで生えていない点に気づいたら鋭いです。このクラスは`on_init`の中でjointの整合性チェックとPub/Subの構築まで一括して済ませています。

`on_init`は`info_.joints`（URDFの`<ros2_control>`タグから渡されるjoint一覧）を1つずつ見て、`voltage`のcommand interfaceを持っているか、`position`/`velocity`のどちらかのstate interfaceを持っているかを検証します（`src/hardware/robot_hardware_chipmd/src/robot_chipmd_hardware.cpp`）。

```cpp
data.has_voltage_command = has_interface(
  joint.command_interfaces, "voltage");
data.has_position_state = has_interface(
  joint.state_interfaces, hardware_interface::HW_IF_POSITION);
data.has_velocity_state = has_interface(
  joint.state_interfaces, hardware_interface::HW_IF_VELOCITY);

if (!data.has_position_state && !data.has_velocity_state) {
  RCLCPP_ERROR(...);
  return hardware_interface::CallbackReturn::ERROR;
}
if (!data.has_voltage_command) {
  RCLCPP_ERROR(...);
  return hardware_interface::CallbackReturn::ERROR;
}
```

つまりこのハードは「voltageコマンドを受けて、position/velocityの状態を返す」という契約を`on_init`の時点で強制しています。URDF側でこの契約を満たさないjointを書くと、`on_init`が`ERROR`を返してcontroller_managerの起動が止まります。ここで一致すべきURDF側の記述が、次に見る`robot_hardware_real.xacro`です。

```xml
<ros2_control name="ExampleChipmdWheelSystem" type="system">
  <hardware>
    <plugin>robot_hardware_chipmd/ExampleChipmdHardware</plugin>
    <param name="feedback_timeout_sec">0.5</param>
    <param name="startup_grace_timeout_sec">5.0</param>
    <param name="tx_publish_period_sec">0.001</param>
  </hardware>
  <joint name="front_left_wheel_joint">
    <command_interface name="voltage"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
  <!-- front_right / rear_left / rear_right も同様 -->
</ros2_control>
```

（出典: `src/description/robot_description/urdf/robot_hardware_real.xacro`）

`<param>`で渡した`feedback_timeout_sec`などの値は、`on_init`内で`info_.hardware_parameters`から`std::stod`で読み出されています。URDFのタグとC++コードの対応は「`<joint>`のname/command_interface/state_interfaceがJointDataの検証条件になり、`<param>`がhardware_parametersとして流れ込む」という形で結びついています。

`export_state_interfaces`/`export_command_interfaces`は、検証済みのjointからinterfaceオブジェクトを作ってcontroller_managerに渡す処理です。

```cpp
std::vector<hardware_interface::StateInterface> ExampleChipmdHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  for (auto& joint : joints_) {
    if (joint.has_position_state) {
      state_interfaces.emplace_back(joint.name, hardware_interface::HW_IF_POSITION, &joint.state_position);
    }
    if (joint.has_velocity_state) {
      state_interfaces.emplace_back(joint.name, hardware_interface::HW_IF_VELOCITY, &joint.state_velocity);
    }
  }
  return state_interfaces;
}
```

ポイントは`&joint.state_position`のようにメンバ変数へのポインタを渡している点です。controller_managerはこのポインタ経由で値を読みに来るので、`read()`が`joint.state_position`を更新すれば、それだけでコントローラ側に新しい値が伝わります。コピーやメッセージ送信を挟まない、ros2_controlのinterfaceの基本設計です。

`read()`と`write()`は次のような役割分担です。

- `read()`: `/motor/state`トピック（ChipMD基板からのエンコーダフィードバック）を受け取るサブスクライバのコールバックが`rx_buffer_`（`realtime_tools::RealtimeBuffer`）に書き込んだ最新値を、リアルタイムループ側から`readFromRT()`で取り出し、タイムアウトしていなければ`joint.state_position`/`state_velocity`に反映する
- `write()`: コントローラが書き込んだ`joint.command_voltage`を`build_tx_command()`で`TxCommand`にまとめ、`tx_buffer_`（`RealtimeThreadSafeBox`）にセットする。実際のPublish処理は`write()`自身ではなく、別で回っている1kHzのウォールタイマ`tx_timer_`が担当している

```cpp
hardware_interface::return_type ExampleChipmdHardware::write(
  const rclcpp::Time&, const rclcpp::Duration&)
{
  if (!tx_buffer_.try_set(build_tx_command(is_latched_fault_))) {
    RCLCPP_DEBUG_THROTTLE(...);
  }
  return hardware_interface::return_type::OK;
}
```

（出典: 同cpp）

read/writeがROS通信を直接叩かず、コールバックとタイマという別経路にPub/Subを分離しているのは、controller_managerのリアルタイムループ（後述する`update_rate: 200`Hz）をROS通信のレイテンシで乱さないための設計です。read/writeは「バッファの読み書きだけをして即戻る」という軽い処理に留め、実際のネットワークI/Oはコールバックとタイマという別の非リアルタイム経路に任せています。

フィードバックが一定時間途絶えると`is_latched_fault_`が立ち、以後`read()`が`hardware_interface::return_type::ERROR`を返してcontroller_manager側にエラーを伝播させる仕組みも実装されています。`on_error`/`on_deactivate`/`on_shutdown`はいずれも`force_zero_output()`を呼び、command_voltageを0にしてから即座に1回publishしています。「止め方」を複数のライフサイクルフックに同じ形で仕込んでおくことで、どの経路で止まっても出力がゼロになることを保証している作りです。

### コントローラの設定と起動

`src/bringup/robot_bringup/config/`にcontrollers.yamlが分かれています。このディレクトリは領域ごとのサブディレクトリ（`hardware/`、`mobility/`、`manipulator/`、`navigation/`、`operation/`、`simulation/`、`udev/`）に整理されています。まず`hardware/base_controllers.yaml`が土台です。

```yaml
controller_manager:
  ros__parameters:
    update_rate: 200

    hardware_components_initial_state:
      unconfigured:
        - ExampleChipmdWheelSystem
        - ExampleChipmdLiftSystem
        - ExampleSts3215Bus0
        - ExampleSts3215Bus1
        - ExampleSts3215Bus2
        - ExampleSolenoidSystem

    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster
```

（出典: `src/bringup/robot_bringup/config/hardware/base_controllers.yaml`）

STS3215のハンドが`Bus0`/`Bus1`/`Bus2`の3コンポーネントに分かれているのは、シリアルサーボを3本のバスに分散させたためです。1バスに全サーボをぶら下げると帯域が足りなくなるので、物理的なバス単位でhardware componentを切っています。ここでも「hardware componentの粒度＝独立にconfigure/activateしたい単位」という原則が効いています。

`update_rate: 200`がcontroller_managerのリアルタイムループの周期（200Hz、つまり5msごとにread→update→write）です。`hardware_components_initial_state`で全コンポーネントを`unconfigured`にしているのは、口頭試問Q3で説明した通り、base.launch.pyだけでは何も起動させないための設計です。

このファイルの上に、サブシステムごとの`mobility/mobility_controllers.yaml`が重なります。

```yaml
controller_manager:
  ros__parameters:
    mobility_controller:
      type: omni_wheel_drive_controller/OmniWheelDriveController
    wheel_pid_controller:
      type: pid_controller/PidController

mobility_controller:
  ros__parameters:
    # ホイールは等間隔・反時計回りの順。最初は front-left（base_link の +X から +45 度）
    wheel_offset: 0.7853981633974483
    wheel_names:
      - wheel_pid_controller/front_left_wheel_joint
      - wheel_pid_controller/rear_left_wheel_joint
      - wheel_pid_controller/rear_right_wheel_joint
      - wheel_pid_controller/front_right_wheel_joint
    robot_radius: 0.29698484809834996
    wheel_radius: 0.05
    open_loop: false
    position_feedback: false      # PIDは位置ではなく計測速度をエクスポートする
    enable_odom_tf: false         # odom -> base_link は odometry_fusion が所有する

wheel_pid_controller:
  ros__parameters:
    dof_names:
      - front_left_wheel_joint
      - front_right_wheel_joint
      - rear_left_wheel_joint
      - rear_right_wheel_joint
    reference_and_state_interfaces:
      - velocity
    command_interface: voltage
    gains:
      front_left_wheel_joint:
        p: 0.04
        i: 0.0
        d: 0.0
        i_clamp_max: 5.0
        i_clamp_min: -5.0
        u_clamp_max: 0.25
        u_clamp_min: -0.25
        antiwindup_strategy: conditional_integration
      # front_right / rear_left / rear_right も同じゲイン
```

（出典: `src/bringup/robot_bringup/config/mobility/mobility_controllers.yaml`）

注目してほしいのは、**上段・下段どちらも自作していない**という点です。`mobility_controller`は`ros2_controllers`標準の`omni_wheel_drive_controller/OmniWheelDriveController`、`wheel_pid_controller`も標準の`pid_controller/PidController`です。4輪オムニのキネマティクスもPID計算も、標準コントローラの組み合わせだけで成立しています。

このリポには`src/control/robot_mobility_controller`という自作の`ChainableControllerInterface`実装も残っていますが、現在どのYAMLからも参照されていません（プラグイン登録の`robot_mobility_controller.xml`だけが残った状態です）。以前は自作コントローラでcmd_vel→4輪変換をやっていて、標準の`omni_wheel_drive_controller`へ置き換えたという経緯です。**「まず標準コントローラで足りないかを確かめ、足りない部分だけ自作する」**という順序の実例として読んでください。逆に、使わなくなった自作プラグインを消さずに残しておくと、後から読む人が「どちらが本番か」で迷います。

`wheel_names`が`wheel_pid_controller/front_left_wheel_joint`のようにコントローラ名のプレフィックス付きになっているのが、[20_ros2_control概要](20_ros2_control概要.md)で触れたコントローラチェーンです。`omni_wheel_drive_controller`はハードウェアのcommand interfaceを直接叩くのではなく、前段の`wheel_pid_controller`が公開しているreference interfaceへ書き込みます。そして`wheel_pid_controller`が`command_interface: voltage`で実ハードへ書きます。cmd_vel → 各輪の目標速度 → PID → voltage、という2段構成です。

ゲインを触る前に単位を意識してください。`wheel_pid_controller`の誤差入力は`velocity`（rad/s）ですが、出力は`voltage`という擬似電圧のcommand interfaceで、`u_clamp_max: 0.25`のように無次元の範囲でクランプされています。`p: 0.04`という小さな値はこの単位変換を兼ねているので、「効きが悪いから10倍にする」という発想でいじると一気に飽和します。

起動順序は`docs/BRINGUP_CHECKLIST.md`と`_subsystem_bringup.py`にまとまっています。各サブシステムのlaunchは次の2段階で構成されています。

1. `hardware_spawner`が対象のハードウェアコンポーネント（例: `ExampleChipmdWheelSystem`）を`--activate`付きで起動し、configure→activateを完了させる
2. `hardware_spawner`のプロセス終了（`OnProcessExit`）を検知して、`spawner`が`joint_state_broadcaster`と対象コントローラ（`mobility_controller`、`wheel_pid_controller`など）を起動する

```python
hardware_spawner = Node(
    package="controller_manager",
    executable="hardware_spawner",
    arguments=[*hardware_components, "--activate", "--controller-manager", CONTROLLER_MANAGER_NAME],
    output="screen",
)
controller_spawner = Node(
    package="controller_manager",
    executable="spawner",
    arguments=["joint_state_broadcaster", *controllers, "--controller-manager", CONTROLLER_MANAGER_NAME],
    output="screen",
)
spawn_controllers_after_hardware = RegisterEventHandler(
    OnProcessExit(target_action=hardware_spawner, on_exit=[controller_spawner]),
)
```

（出典: `src/bringup/robot_bringup/launch/_subsystem_bringup.py`）

コントローラは対応するハードウェアコンポーネントがactiveになってからでないとinterfaceを掴めないため、この順序を守らないとspawnerがinterface未検出で失敗します。`joint_state_broadcaster`をサブシステムのlaunchの中でスポーンしているのも意図的な設計で、複数のサブシステムを個別に立ち上げると二重スポーンになるため、全部同時に使いたいときは個別launchではなく`robot.launch.py`（全コンポーネント活性化＋spawnerを一度だけ実行する統合エントリポイント）を使います。

### 動かして確認

実機なしでも、mockハードウェアでcontroller_managerの挙動は確認できます。`docs/BRINGUP_CHECKLIST.md`の手順に沿って進めます。

```bash
colcon build
source install/setup.bash
# ドライラン（mock、CANなし、IMUなし）
ros2 launch robot_bringup robot.launch.py \
    use_mock_hardware:=true start_board_io:=false start_dm_imu:=false
```

`use_mock_hardware:=true`だけでは足りません。`start_board_io`と`start_dm_imu`は既定が`true`なので、CAN gatewayとDM-IMUのノードが実デバイスを開こうとして失敗します。実機が手元にないときは3つセットで渡してください（このコマンドは`robot.launch.py`冒頭のdocstringに「ドライラン (mock、CAN なし)」として書かれているものです）。

別ターミナルで、動いているコントローラの一覧を見ます。

```bash
ros2 control list_controllers
```

mock構成での実際の出力は次のとおりです。

```text
solenoid_command_controller robot_solenoid_command_controller/ExampleSolenoidCommandController  active
lift_position_controller    robot_lift_position_controller/ExampleLiftPositionController        active
hand_controller             robot_hand_controller/ExampleHandController                         active
mobility_controller         omni_wheel_drive_controller/OmniWheelDriveController                    active
wheel_pid_controller        pid_controller/PidController                                            active
joint_state_broadcaster     joint_state_broadcaster/JointStateBroadcaster                           active
```

6つすべてが`active`になっていれば想定どおりです。左列がインスタンス名、中列が`type`（=プラグインのクラス名）です。**自作コントローラは3つ（solenoid / lift / hand）だけで、足回りの2つは標準コントローラ**であることがこの1行1行から読み取れます。

ハードウェアコンポーネント側を見るには次の2つを使います。

```bash
ros2 control list_hardware_components
ros2 control list_hardware_interfaces
```

`list_hardware_components`の出力（抜粋）はこうなります。

```text
Hardware Component 1
	name: ExampleSts3215Bus0
	type: system
	plugin name: mock_components/GenericSystem
	state: id=3 label=active
	read/write rate: 200 Hz
	is_async: False
	command interfaces
		right_1_tip_joint/position [available] [claimed]
		right_0_tip_joint/position [available] [claimed]
		...
```

`read/write rate: 200 Hz`が`base_controllers.yaml`の`update_rate: 200`に対応しています。`[claimed]`はそのinterfaceを何らかのコントローラが掴んでいる印で、コントローラを`active`にしていないと`[available]`だけになります。

`list_hardware_interfaces`のwheel関連を抜き出すと、コントローラチェーンの姿がはっきり見えます。

```text
command interfaces
	base_to_lift/voltage [available] [claimed]
	front_left_wheel_joint/velocity [available] [claimed]        ← mock のハード（実機は voltage）
	front_right_wheel_joint/velocity [available] [claimed]
	rear_left_wheel_joint/velocity [available] [claimed]
	rear_right_wheel_joint/velocity [available] [claimed]
	wheel_pid_controller/front_left_wheel_joint/velocity [available] [claimed]   ← PIDが公開する
	wheel_pid_controller/front_right_wheel_joint/velocity [available] [claimed]     reference interface
	wheel_pid_controller/rear_left_wheel_joint/velocity [available] [claimed]
	wheel_pid_controller/rear_right_wheel_joint/velocity [available] [claimed]
state interfaces
	front_left_wheel_joint/position
	front_left_wheel_joint/velocity
	...
```

注目してほしいのは、**command interfaceが2種類ある**ことです。プレフィックスなしの`front_left_wheel_joint/velocity`がハードウェアのinterface、`wheel_pid_controller/`付きのほうが`wheel_pid_controller`自身が公開しているreference interfaceです。`mobility_controller`は後者に書き込み、`wheel_pid_controller`が前者に書き込みます。`mobility_controllers.yaml`の`wheel_names`に`wheel_pid_controller/front_left_wheel_joint`と書いてあったのは、まさにこの名前です。YAMLの設定とこの出力を突き合わせると、チェーンが意図どおり繋がっているかを目で確認できます。

`base_to_lift`だけはmockでも`voltage`のままです。mockで`velocity`に変えているのはwheelの4関節だけで、リフトは位置制御なので`GenericSystem`の積分を必要としないからです。実機の`robot_hardware_real.xacro`の宣言と見比べる癖をつけてください。宣言と実際のinterfaceが食い違っていたら、まずURDFかC++側の検証ロジックを疑うことになります。

指令を送って反応を見るには`/cmd_vel`をpublishします。

```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/TwistStamped \
  "{header: {stamp: now, frame_id: base_link}, twist: {linear: {x: 0.1, y: 0.0, z: 0.0}, angular: {z: 0.0}}}" \
  --rate 20 --times 20
```

（出典: `docs/BRINGUP_CHECKLIST.md` L34）

型が`Twist`ではなく**`TwistStamped`**であることに注意してください。`omni_wheel_drive_controller`はタイムスタンプ付きを受け取ります。`Twist`で投げても購読者の型が合わないので何も起きず、しかもエラーも出ないので気づきにくいところです。

mockハードウェアなので実際にモータは回りませんが、`/odom`をechoすると積算が進んでいくのが見えます。手元で`linear.x: 0.3`を4秒ほど流したときの`/odom`はこうなりました。

```yaml
    position:
      x: 1.8570142507553127
      y: -2.1468619681095987e-16
      z: 0.0
```

`x`だけが増えて`y`がほぼゼロ（`-2.1e-16`は浮動小数の誤差）です。つまり「cmd_vel → omniコントローラのキネマティクス → PIDチェーン → mockハードのvelocity → state interface → wheel odometry」という経路が全部繋がっています。`/joint_states`の周期も確認しておくとよいです。

```
$ ros2 topic hz /joint_states
average rate: 199.998
	min: 0.005s max: 0.005s std dev: 0.00012s window: 201
```

`update_rate: 200`のとおり5ms周期で回っています。この数字がガタつくようなら、`update_rate`に対してCPUが足りていないか、read/writeのどこかでブロッキングしているサインです。

> 注意: この`/cmd_vel`直接publishは`robot.launch.py`でコントローラを単体確認するときだけの手順です。`manual.launch.py`や`auto.launch.py`では`/cmd_vel`をmux（`robot_command_arbiter`）が所有するので、同じトピックへ直接publishしてはいけません（`docs/BRINGUP_CHECKLIST.md` L42-43）。手動指令と自律指令の調停を壊してしまいます。

**実機のCAN接続がある場合のみ**、`use_mock_hardware:=false`に切り替えて`/cmd_vel`で実際に足回りが動くことを確認します（実機確認の手順とチェック項目は`docs/BRINGUP_CHECKLIST.md`の4節を参照してください）。

**練習問題**: `mobility_controllers.yaml`の`wheel_pid_controller`のゲインには`u_clamp_max: 0.25`という制限がついています。これがなぜ`[-1, 1]`ではなく`0.25`なのか、`mobility_controller`の役割と合わせて考えてください。

<details markdown="1"><summary>解答</summary>

`wheel_pid_controller`はcommand interfaceとして`voltage`を`[-1, 1]`の擬似電圧で出力できますが、`u_clamp_max`を`0.25`に絞っているのは、PID単体の出力上限を意図的に低く抑えるチューニングです。仮に`u_clamp_max`が`1.0`のままだと、速度の誤差が大きいときにPIDがフルパワーの電圧指令を出してしまい、急な挙動や過電流のリスクにつながります。`mobility_controller`側で計算したwheel velocity referenceに対して、PID側の出力を抑えめにしておくことで、キネマティクス計算の誤差やタイヤの空転などが起きても出力が暴れにくい構成にしています。

`[-1, 1]`という擬似電圧の範囲は`config/hardware/mock_dynamics_overrides.yaml`のコメント（「実機の ChipMD ハードウェアは [-1, 1] にクランプされた疑似電圧指令を要求する」）で確認できます。`p: 0.04`も`u_clamp_max: 0.25`も実機で追い込む暫定値なので、いじる前に`docs/PID_TUNING.md`を読んでください。

</details>

## 発展

### mock_componentsでハード無しにテストする

`robot_hardware_mock.xacro`は`mock_components/GenericSystem`を使ってExampleChipmdWheelSystemなどを差し替えています。ここで面白いのは、wheelのcommand interfaceが実機用の`voltage`ではなく`velocity`に変わっている点です。

```xml
<!-- Wheels use a "velocity" command interface here instead of real
     hardware's "voltage", so mock_components/GenericSystem's
     calculate_dynamics can integrate it into position/velocity state
     (it only understands a "velocity"-named command). -->
<ros2_control name="ExampleChipmdWheelSystem" type="system">
  <hardware>
    <plugin>mock_components/GenericSystem</plugin>
    <param name="mock_sensor_commands">false</param>
    <param name="calculate_dynamics">true</param>
  </hardware>
  <joint name="front_left_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
  ...
</ros2_control>
```

（出典: `src/description/robot_description/urdf/robot_hardware_mock.xacro`）

`GenericSystem`の`calculate_dynamics`は`velocity`という名前のcommand interfaceしか積分してくれないため、実機の`voltage`のままではmock上で関節が回りません。そのため`config/hardware/mock_dynamics_overrides.yaml`という差分ファイルが用意されており、`use_mock_hardware:=true`のときだけ`mobility_controllers.yaml`の後に読み込まれます。「同じコントローラ設定を実機とmockで完全に共有する」のではなく、食い違う部分だけを差分ファイルで吸収するという現実的なやり方です。実機とmockでハードウェアコンポーネント名を揃えているのも、この差し替えを`base_controllers.yaml`や各launchの記述を変えずに成立させるための工夫です。

このファイルが上書きしているのは`command_interface`だけではありません。ここが面白いところなので中身を見てください。

```yaml
wheel_pid_controller:
  ros__parameters:
    command_interface: velocity
    enable_feedforward: true
    gains:
      front_left_wheel_joint:
        p: 0.0                  # 実機は 0.04
        u_clamp_max: 25.0       # 実機は 0.25
        feedforward_gain: 1.0
      # 他3輪も同じ
```

**Pゲインを0にして、フィードフォワード1.0だけにしています。** 理由はファイル内のコメントに書かれています。`GenericSystem`はvelocity指令をプラント遅れなしにそのままフィードバックへミラーするので、P制御だけのループはリファレンスとゼロの間で交互に振動してしまいます。また実機のゲイン（`p=0.04`、`u_clamp=±0.25`）は擬似電圧が飽和しないよう調整された値なので、velocity指令として使うと小さすぎて動きが見えません。だからmockでは「指令をそのまま通す」ほうが素直だという判断です。

ここから学べるのは、**mockは「実機と同じ設定で動くこと」を目指す場所ではない**ということです。mockの目的はlaunchの配線・interfaceの整合・odometryの流れを確かめることなので、そのために制御の中身を割り切って変えるのは正しい判断です。逆にmockでゲイン調整をしても実機には持っていけません。

> コラム: このファイルには「`rcl_yaml_param_parser`はYAMLのアンカー/エイリアスに対応していないため、`&`/`*`を使わずにホイールごとに同じブロックを繰り返している」というコメントがあります。ROS 2のパラメータYAMLは一般的なYAMLパーサではなく専用のC実装で読まれるため、アンカーのような便利機能が使えません。4輪ぶん同じブロックをコピペしているのは手抜きではなく、パーサの制約です。実際にアンカーを書いたparams fileを渡すと、こう落ちます。
>
> ```
> what():  failed to initialize rcl: Couldn't parse params file: '--params-file ...'.
>          Error: Will not support aliasing at line 7
> ```
>
> `python3 -c "import yaml; yaml.safe_load(...)"`では問題なく読めてしまうので、「YAMLとしては正しいのにROS 2だけが受け付けない」という形でハマります。`Will not support aliasing`が出たらこれです。

### gz_ros2_controlでシミュレータに差し替える

mockは「関節が数値上動く」だけで、地面との摩擦も物体との接触もありません。3つ目の選択肢が`robot_hardware_sim.xacro`で、こちらは`gz_ros2_control`を使ったGazebo Harmonic連携です。`use_sim_hardware:=true`（または`simulation.launch.py`）で選ばれます。

```xml
<xacro:macro name="robot_hardware_sim" params="prefix enabled_hand:=all sts3215_bus_config">
  <ros2_control name="GazeboSimSystem" type="system">
    <hardware>
      <plugin>gz_ros2_control/GazeboSimSystem</plugin>
    </hardware>
    <joint name="front_left_wheel_joint">
      <command_interface name="velocity">
        <param name="min">-25.0</param>
        <param name="max">25.0</param>
      </command_interface>
      <state_interface name="position"/>
      <state_interface name="velocity"/>
      <state_interface name="effort"/>
    </joint>
    ...
  </ros2_control>

  <gazebo>
    <plugin filename="libgz_ros2_control-system"
            name="gz_ros2_control::GazeboSimROS2ControlPlugin">
      <parameters>$(find robot_bringup)/config/simulation/gazebo_controllers.yaml</parameters>
      <hold_joints>true</hold_joints>
    </plugin>
  </gazebo>
</xacro:macro>
```

（出典: `src/description/robot_description/urdf/robot_hardware_sim.xacro`）

ここで押さえたいのは、**3つの`<ros2_control>`定義（real / mock / sim）が同じjoint名を公開している**という点です。ファイル冒頭のコメントにも "The controller facing wheel joint names remain identical to the real robot." と書かれています。joint名が同じなので、`base_controllers.yaml`や各サブシステムlaunchは書き換えずに、URDF側の差し替えだけで実機・mock・Gazeboを行き来できます。これが[19_URDFの書き方](19_URDFの書き方.md)で扱った「URDFはハードとソフトの契約書」という話の実例です。

一方でsimは`voltage`ではなく`velocity`のcommand interfaceを使い、`gazebo_controllers.yaml`という別のcontroller設定を読みます。Gazeboの物理エンジンは擬似電圧を解釈できないので、mockと同じ理由でinterfaceを差し替える必要があるわけです。

なお`gz_ros2_control`とは別に、Gazebo側のプラグインで吸盤の吸着を再現しています。`gz-sim-detachable-joint-system`を吸盤ごとに宣言し、`/simulation/suction/<channel>/attach`と`/detach`というトピックで物体をくっつけたり離したりします。ros2_controlの枠外で、シミュレータ固有の機能をトピック越しに使っている例です。

> 補足: ADR 0003（`docs/adr/0003-do-not-vendor-sim-repo.md`）は「シミュレータリポジトリは当面submoduleにしない」という判断で、これは現在も`Accepted`のままです。ただし結果として、Gazebo連携は別リポを取り込むのではなく**このリポ内に自前で実装する**方向に進みました。ADRの判断そのものは有効ですが、「だからGazebo連携は未実装」ではありません。ADRを読むときは、決定の日付とリポジトリの現状を必ず突き合わせてください。

## おわりに

自作hardware_interfaceを書くこと自体は、ros2_controlの中では一番手間のかかる部分ですが、書いてしまえばコントローラ側（`diff_drive_controller`や`pid_controller`のような標準コントローラ）は差し替え可能な形で使えます。このリポがwheel/lift/hand/solenoidの4系統すべてをros2_controlに統合した理由も、この「hardware_interfaceだけ書けば残りは共通の仕組みに乗る」という保守性にあります。わからなければ先輩に聞きましょう。

次は[21_センサ統合](21_センサ統合.md)で、LiDARやIMUをROS 2側に取り込む方法を扱います。

## 資料

- [ros2_control公式（control.ros.org）: Writing a Hardware Component](https://control.ros.org/jazzy/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html)
- [ros2_control公式（control.ros.org）トップ](https://control.ros.org/jazzy/index.html)
- [20_ros2_control概要](20_ros2_control概要.md)
- [21_センサ統合](21_センサ統合.md)
