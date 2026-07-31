# ROS2講習19: URDFの書き方

## はじめに

この記事を終えると、2輪台車のURDFを自分で書き、RViz2上に自分のロボットを表示できるようになります。

前提は[18_TF2と座標系](18_TF2と座標系.md)を読み終えていることです。URDFはロボットのTFツリーを自動生成するための入力そのものなので、TFの仕組みを知らずにURDFを書くと「何のために書いているか」がわからなくなります。

## 講習目標

- URDFがなぜ必要か（TFツリーの自動生成・可視化・ros2_controlの前提）を自分の言葉で説明できる
- linkの3要素（visual/collision/inertial）とjointの主要type（fixed/continuous/revolute/prismatic）を書ける
- 最小の2輪台車URDFを自分で書き、robot_state_publisherとjoint_state_publisher_guiでRViz2に表示できる
- xacroでプロパティ化・マクロ化された記述に書き換えられる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jaliscoがセットアップ済みの環境（[02_環境構築](02_環境構築.md)完了）
- `ros-jazzy-joint-state-publisher-gui`、`ros-jazzy-xacro`（未インストールなら`sudo apt install ros-jazzy-joint-state-publisher-gui ros-jazzy-xacro`）
- RViz2が起動できる環境（GUIが出ること。SSH経由の受講なら事前にX転送かVNCを確認しておく）
- 練習用ワークスペース（[10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)を先に済ませておく）

### 時間配分の目安

- URDFの目的とTFの関係の説明: 10分
- link/jointの文法説明: 15分
- 2輪台車URDFを書く課題: 25分
- robot_state_publisher + RViz2表示: 10分
- xacro化: 15分
- 口頭試問: 10分

### 口頭試問

**Q1. URDFを書かずにロボットのモデルをROS2に登録しなかった場合、何ができなくなりますか。**

模範解答: TFツリーが自動生成されないため、`base_link`からセンサや車輪までの座標変換を全部手動でpublishしなければならなくなる。RViz2でロボットの形が表示できず、デバッグ時に「センサがどこを向いているか」が見えない。ros2_controlはURDFの`<joint>`情報を前提にハードウェアインターフェースを構成するため、URDFがないとコントローラのロード自体ができない。

**Q2. `<link>`の中のvisual、collision、inertialはそれぞれ何のためにありますか。1つだけ省略してよいとしたらどれですか。**

模範解答: visualは見た目（RViz2やGazeboでの描画）、collisionは当たり判定（ナビゲーションのコストマップやGazeboの物理演算）、inertialは質量・慣性モーメント（物理シミュレーションの動力学計算）のために使う。実機を動かすだけでシミュレーションをしないなら、inertialは省略しても動く（ros2_controlの一部プラグインは警告を出すことがあるが致命的ではない）。ただしGazeboで物理シミュレーションをする場合はinertialが必須。

**Q3. `continuous`と`revolute`の違いは何ですか。駆動輪にはどちらを使うべきですか。**

模範解答: `revolute`は`<limit>`で上限・下限が必須で、その範囲内でしか回転できない。`continuous`は制限なく無限に回転できる。駆動輪は前進し続けるので回転角に上限がなく、`continuous`を使う。アームの関節のように可動範囲が決まっているものには`revolute`を使う。

## 本文

### 学習内容：URDFは何のために書くのか

学習内容：URDFがTFツリー・可視化・ros2_controlの前提になっていることを理解する。

準備：[18_TF2と座標系](18_TF2と座標系.md)を読み終えていること。

内容：

URDF（Unified Robot Description Format）は、ロボットの形状と関節構造をXMLで記述したファイルです。「このロボットにはこういう部品（link）があり、それらはこういう関節（joint）で繋がっている」という情報だけを持っていて、それ自体は何も動かしません。

このURDFを`robot_state_publisher`に読み込ませると、joint情報からlink間の座標変換（TF）が自動的に計算されてpublishされます。[18_TF2と座標系](18_TF2と座標系.md)で「TFは誰かが計算してpublishしないと生まれない」と説明しましたが、固定された部品間や関節で繋がった部品間のTFを毎回自分で計算するのは非現実的です。URDFを書けば、この計算をROS2側に任せられます。

URDFが前提になっている先は3つあります。

- **TFツリー**: `robot_state_publisher`がURDFのjoint構造からTFを自動生成する
- **可視化**: RViz2はURDFを読んでロボットの3Dモデルを描画する。TFが正しくても形が入っていなければ棒人間のような表示にしかならない
- **ros2_control**: [20_ros2_control概要](20_ros2_control概要.md)で扱うが、ros2_controlはURDFの`<joint>`と`<ros2_control>`タグを読んでハードウェアインターフェースを構成する。URDFの関節名や可動範囲が間違っていると、コントローラが正しく動かない

つまりURDFは「見た目のための飾り」ではなく、この先の講習全部が読み込む共通の入力データです。

### 学習内容：linkの文法

学習内容：`<link>`の中の visual/collision/inertial を書けるようになる。

準備：特になし。

内容：

`<link>`はロボットを構成する1つの剛体部品を表します。中身は大きく3つに分かれます。

```xml
<link name="base_link">
  <visual>
    <geometry>
      <box size="0.3 0.2 0.1"/>
    </geometry>
    <origin xyz="0 0 0" rpy="0 0 0"/>
    <material name="blue">
      <color rgba="0 0 0.8 1"/>
    </material>
  </visual>
  <collision>
    <geometry>
      <box size="0.3 0.2 0.1"/>
    </geometry>
    <origin xyz="0 0 0" rpy="0 0 0"/>
  </collision>
  <inertial>
    <mass value="1.0"/>
    <inertia ixx="0.01" ixy="0.0" ixz="0.0" iyy="0.01" iyz="0.0" izz="0.01"/>
  </inertial>
</link>
```

- `visual`: 見た目。`geometry`に`box`/`cylinder`/`sphere`/`mesh`のいずれかを指定する
- `collision`: 当たり判定用の形状。visualと同じ形を指定することが多いが、計算を軽くするために簡略化した形状（複雑なメッシュの代わりに円柱や箱）を使うこともある
- `inertial`: 質量と慣性モーメント。シミュレーションの物理計算に使う

`origin`は`xyz`（並進, メートル）と`rpy`（roll-pitch-yaw, **ラジアン**）で位置と姿勢を指定します。度数ではなくラジアンなので、90度回したいなら`1.5708`と書く必要があります。ここを度数のまま書いてしまう間違いが非常に多いので、電卓か`python3 -c "import math; print(math.pi/2)"`で確認する癖をつけてください。

> コラム: `mesh`を使う場合は`<mesh filename="package://パッケージ名/meshes/base.stl"/>`のように`package://`スキームで指定します。絶対パスや相対パスを直接書くと、他の人の環境やビルド後のインストール先ではファイルが見つからずに読み込みエラーになります。パッケージ名がタイポしていたり、`package.xml`にmeshesディレクトリのインストール設定がなかったりすると同じエラーが出るので、エラーが出たら両方を確認してください。

### 学習内容：jointの文法

学習内容：linkとlinkを繋ぐ`<joint>`を書けるようになる。

準備：特になし。

内容：

linkだけでは部品がバラバラに浮いているだけです。`<joint>`で親子関係と繋がり方を定義します。

```xml
<joint name="wheel_left_joint" type="continuous">
  <parent link="base_link"/>
  <child link="wheel_left_link"/>
  <origin xyz="0.0 0.15 -0.05" rpy="-1.5708 0 0"/>
  <axis xyz="0 0 1"/>
</joint>
```

- `parent`/`child`: どのlinkとどのlinkを繋ぐか。ここで指定した`parent`が親フレームになる
- `origin`: 親linkの座標系から見た、子linkの原点の位置と姿勢。**jointのoriginは「子の初期位置」を決めるもので、joint自体はここを軸にして動くわけではない**（回転軸は次の`axis`で決まる）
- `axis`: 回転・並進の軸方向（`type`が`fixed`の場合は不要）

`type`は主に次の6種類です。

| type | 意味 |
|---|---|
| fixed | 固定。相対的に動かない |
| continuous | 軸周りに無制限に回転する |
| revolute | 軸周りに回転するが`<limit>`で上下限がある |
| prismatic | 軸方向にスライドする |
| floating | 6自由度（回転+並進）で自由に動く |
| planar | 平面内を並進する |

`revolute`と`prismatic`は`<limit>`が必須です。

```xml
<joint name="arm_joint" type="revolute">
  <parent link="base_link"/>
  <child link="arm_link"/>
  <origin xyz="0 0 0.1" rpy="0 0 0"/>
  <axis xyz="0 1 0"/>
  <limit lower="-1.57" upper="1.57" effort="10.0" velocity="1.0"/>
</joint>
```

`lower`/`upper`はラジアン（`prismatic`ならメートル）、`effort`は最大トルク（N・m）、`velocity`は最大角速度（rad/s）です。駆動輪は無限に回り続けるので`continuous`を使い、`<limit>`は書きません。

### 学習内容：最小の2輪台車URDFを書く

学習内容：ここまでの文法を組み合わせて、1つの動く（見える）ロボットモデルを作る。

準備：ワークスペースに`description`パッケージを作っておきます。

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake my_robot_description
mkdir my_robot_description/urdf
```

内容：

`my_robot_description/urdf/my_robot.urdf`を作成します。台車本体と左右の車輪、キャスターの4リンク構成にします。

```xml
<?xml version="1.0"?>
<robot name="my_robot">

  <link name="base_link">
    <visual>
      <geometry>
        <box size="0.3 0.2 0.1"/>
      </geometry>
      <origin xyz="0 0 0" rpy="0 0 0"/>
      <material name="blue">
        <color rgba="0 0 0.8 1"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <box size="0.3 0.2 0.1"/>
      </geometry>
      <origin xyz="0 0 0" rpy="0 0 0"/>
    </collision>
    <inertial>
      <mass value="1.0"/>
      <inertia ixx="0.01" ixy="0.0" ixz="0.0" iyy="0.01" iyz="0.0" izz="0.01"/>
    </inertial>
  </link>

  <link name="wheel_left_link">
    <visual>
      <geometry>
        <cylinder radius="0.05" length="0.02"/>
      </geometry>
      <material name="black">
        <color rgba="0.1 0.1 0.1 1"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <cylinder radius="0.05" length="0.02"/>
      </geometry>
    </collision>
    <inertial>
      <mass value="0.1"/>
      <inertia ixx="1e-4" ixy="0.0" ixz="0.0" iyy="1e-4" iyz="0.0" izz="1e-4"/>
    </inertial>
  </link>

  <link name="wheel_right_link">
    <visual>
      <geometry>
        <cylinder radius="0.05" length="0.02"/>
      </geometry>
      <material name="black">
        <color rgba="0.1 0.1 0.1 1"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <cylinder radius="0.05" length="0.02"/>
      </geometry>
    </collision>
    <inertial>
      <mass value="0.1"/>
      <inertia ixx="1e-4" ixy="0.0" ixz="0.0" iyy="1e-4" iyz="0.0" izz="1e-4"/>
    </inertial>
  </link>

  <joint name="wheel_left_joint" type="continuous">
    <parent link="base_link"/>
    <child link="wheel_left_link"/>
    <origin xyz="0.0 0.15 -0.05" rpy="-1.5708 0 0"/>
    <axis xyz="0 0 1"/>
  </joint>

  <joint name="wheel_right_joint" type="continuous">
    <parent link="base_link"/>
    <child link="wheel_right_link"/>
    <origin xyz="0.0 -0.15 -0.05" rpy="-1.5708 0 0"/>
    <axis xyz="0 0 1"/>
  </joint>

</robot>
```

車輪の`origin`の`rpy="-1.5708 0 0"`は、シリンダーの標準の向き（軸方向がZ）を90度回してY軸方向（左右方向）に軸を向けるためのものです。ここを省略すると車輪が正面を向いたまま回らない・変な向きに描画される、という詰まりが必ず起きます。

**練習問題**: 上記のURDFにキャスター（`caster_link`、球体、`fixed`ジョイントで`base_link`前方下に固定）を追加してください。

<details><summary>解答</summary>

```xml
<link name="caster_link">
  <visual>
    <geometry>
      <sphere radius="0.03"/>
    </geometry>
    <material name="grey">
      <color rgba="0.5 0.5 0.5 1"/>
    </material>
  </visual>
  <collision>
    <geometry>
      <sphere radius="0.03"/>
    </geometry>
  </collision>
  <inertial>
    <mass value="0.05"/>
    <inertia ixx="1e-5" ixy="0.0" ixz="0.0" iyy="1e-5" iyz="0.0" izz="1e-5"/>
  </inertial>
</link>

<joint name="caster_joint" type="fixed">
  <parent link="base_link"/>
  <child link="caster_link"/>
  <origin xyz="0.12 0 -0.07" rpy="0 0 0"/>
</joint>
```

キャスターは駆動しないので`fixed`で十分です。実際のキャスターは自由に転がりますが、シミュレーション上でその自由度まで再現すると複雑になるため、簡易モデルでは`fixed`で済ませることが多いです。

</details>

### 学習内容：RViz2で表示する

学習内容：`robot_state_publisher`と`joint_state_publisher_gui`で自分のURDFを可視化する。

準備：`my_robot.urdf`が書き終わっていること。

内容：

まず`robot_state_publisher`にURDFを渡して起動します。

```bash
ros2 run robot_state_publisher robot_state_publisher --ros-args -p robot_description:="$(xacro ~/ros2_ws/src/my_robot_description/urdf/my_robot.urdf)"
```

URDFが純粋なXML（xacroを使っていない）でも`xacro`コマンドを通して問題ありません。プレーンなURDFはxacroにとっても有効な入力です。

`continuous`/`revolute`/`prismatic`のjointがある場合、そのjoint角度を誰かがpublishしないとTFが完成しません。演習用には`joint_state_publisher_gui`でスライダーを動かして仮の値を送ります。

```bash
ros2 run joint_state_publisher_gui joint_state_publisher_gui
```

最後にRViz2を起動し、`Fixed Frame`を`base_link`に設定して、`RobotModel`ディスプレイを追加します。`Description Topic`は`/robot_description`のままで大丈夫です。

```bash
rviz2
```

車輪のスライダーを動かすと、RViz2上で車輪だけが回転することを確認してください。台車本体は動かず、車輪だけが`wheel_left_joint`/`wheel_right_joint`の軸周りに回るのが正しい挙動です。

ヒント：RobotModelが表示されない場合は、`Fixed Frame`の指定ミスか、`Description Topic`が実際にpublishされているトピック名と一致していないことが多いです。`ros2 topic list`で`/robot_description`が出ているか確認してください。

> コラム: `joint_state_publisher_gui`はあくまで演習・デバッグ用です。実機では[20_ros2_control概要](20_ros2_control概要.md)で扱う`ros2_control`のコントローラが実際のエンコーダ値から`/joint_states`をpublishするので、GUIは使いません。

### 学習内容：xacroで変数化・マクロ化する

学習内容：寸法をプロパティにまとめ、繰り返し部分をマクロにする。

準備：特になし。

内容：

先ほどのURDFは車輪のlinkとjointがほぼ同じ内容の繰り返しでした。さらに、車輪半径やトレッド（左右の車輪間距離）のような寸法が複数箇所にハードコードされていて、後から変えるときに全部探して直す必要があります。これをxacroで解決します。拡張子を`.urdf.xacro`にします。

```xml
<?xml version="1.0"?>
<robot name="my_robot" xmlns:xacro="http://www.ros.org/wiki/xacro">

  <xacro:property name="wheel_radius" value="0.05"/>
  <xacro:property name="wheel_length" value="0.02"/>
  <xacro:property name="tread" value="0.3"/>

  <link name="base_link">
    <visual>
      <geometry>
        <box size="0.3 0.2 0.1"/>
      </geometry>
      <material name="blue">
        <color rgba="0 0 0.8 1"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <box size="0.3 0.2 0.1"/>
      </geometry>
    </collision>
    <inertial>
      <mass value="1.0"/>
      <inertia ixx="0.01" ixy="0.0" ixz="0.0" iyy="0.01" iyz="0.0" izz="0.01"/>
    </inertial>
  </link>

  <xacro:macro name="wheel" params="prefix reflect">
    <link name="wheel_${prefix}_link">
      <visual>
        <geometry>
          <cylinder radius="${wheel_radius}" length="${wheel_length}"/>
        </geometry>
        <material name="black">
          <color rgba="0.1 0.1 0.1 1"/>
        </material>
      </visual>
      <collision>
        <geometry>
          <cylinder radius="${wheel_radius}" length="${wheel_length}"/>
        </geometry>
      </collision>
      <inertial>
        <mass value="0.1"/>
        <inertia ixx="1e-4" ixy="0.0" ixz="0.0" iyy="1e-4" iyz="0.0" izz="1e-4"/>
      </inertial>
    </link>

    <joint name="wheel_${prefix}_joint" type="continuous">
      <parent link="base_link"/>
      <child link="wheel_${prefix}_link"/>
      <origin xyz="0.0 ${reflect * tread / 2} ${-wheel_radius}" rpy="-1.5708 0 0"/>
      <axis xyz="0 0 1"/>
    </joint>
  </xacro:macro>

  <xacro:wheel prefix="left" reflect="1"/>
  <xacro:wheel prefix="right" reflect="-1"/>

</robot>
```

`<xacro:property>`で宣言した値は`${wheel_radius}`のように参照できます。トレッドや車輪半径をロボコンの実機に合わせて変えたいときは、この3行を直すだけで全部の箇所に反映されます。

`<xacro:macro>`は左右の車輪でほぼ同一だった記述を1つにまとめたものです。`params`に渡した`prefix`と`reflect`で、リンク名（`wheel_left_link`/`wheel_right_link`）とY方向のオフセットの符号を切り替えています。呼び出し側は`<xacro:wheel prefix="left" reflect="1"/>`のように2行で済み、寸法の変更や車輪追加のたびに全リンクを手書きする必要がなくなります。

xacroファイルを展開して純粋なURDFを確認したい場合は、次のコマンドで見られます。

```bash
xacro my_robot.urdf.xacro > /tmp/my_robot_expanded.urdf
```

`robot_state_publisher`に渡すときは展開後のURDFでも`.xacro`ファイルそのままでも構いません。`xacro`コマンドを通す前提であればどちらでも動きます。

> コラム: URDFを書くときは、各タグの引数をリファレンス資料で確認しながら作業するのが効率的です。

## 発展

xacroにはif分岐（`<xacro:if>`）や数式（`${}`内の四則演算）も使えるので、シミュレーション用と実機用でモデルを切り替えるといった応用ができます。また、SolidWorksやFusion360のCADモデルから`sw_urdf_exporter`や`fusion2urdf`のようなツールでmesh付きのURDFを自動生成する方法もあります。手で全リンクの慣性テンソルを計算するのは大変なので、CADの質量特性ツールから値を持ってくるか、エクスポータに計算させるのが現実的です。ここでは名前を知っておく程度に留め、深掘りは別記事に譲ります。


## おわりに

URDFはこの先の講習全部の入力になるファイルです。特にトレッドとホイール半径は、[20_ros2_control概要](20_ros2_control概要.md)の差動駆動コントローラと[22_自己位置推定](22_自己位置推定の考え方.md)のオドメトリ計算の両方が直接使う数値なので、実機の寸法を正確に測ってから入れてください。ここが1mmでも違うと、オドメトリがずれ続けて自己位置推定の精度に響きます。わからなければ先輩に聞きましょう。

次は[20_ros2_control概要](20_ros2_control概要.md)で、このURDFを使って実際にモータを動かす仕組みを扱います。

## 資料

- [ROS 2 Documentation: Jazzy — URDF Main](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/URDF/URDF-Main.html)
- [ROS 2 Documentation: Jazzy — Using Xacro to Clean Up a URDF File](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/URDF/Using-Xacro-to-Clean-Up-a-URDF-File.html)
- [18_TF2と座標系](18_TF2と座標系.md)
- [20_ros2_control概要](20_ros2_control概要.md)
