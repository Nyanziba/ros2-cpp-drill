# ROS2講習15: パラメータとlaunchの実践

## はじめに

この記事を終えると、`max_speed`のようなパラメータをノードに実装し、YAMLファイルとlaunchファイルの組み合わせで起動できるようになります。さらに`ros2 param set`で実行中に値を変えたとき、ノードの内部動作が実際に切り替わる仕組みも作れるようになります。

[07_パラメータ](07_パラメータ.md)で「`ros2 param set`で値を変えたのに動作が変わらない」という現象を扱いました。あれはノード側がパラメータ変更を受け取るコールバックを実装していなかったからです。ここでその実装を回収します。[09_launchとros2_bag](09_launchとros2_bag.md)のlaunchファイルはremapを埋め込んだだけの最小構成でしたが、ここでは`DeclareLaunchArgument`とYAML注入まで踏み込みます。

前提は[12_Pythonでpub_subを書く](12_Pythonでpub_subを書く.md)まで完了していることです。パラメータを持つノード自体はPythonのpub/subノードとほぼ同じ形なので、rclpyでノードを書けることが前提になります。

## 講習目標

- `declare_parameter`/`get_parameter`でパラメータを持つノードを書ける
- `add_on_set_parameters_callback`で実行中のパラメータ変更を検知し、内部変数に反映できる
- パラメータ用YAMLファイルをパッケージにinstallし、launchファイルから`parameters=[...]`で読み込ませられる
- `DeclareLaunchArgument`/`LaunchConfiguration`でlaunch引数を扱える

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jaliscoがセットアップ済みの環境
- [10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)で作成済みのワークスペース
- 本講習用パッケージ（Python、ament_python）。当日新規作成でもよいが、`ros2 pkg create --build-type ament_python speed_param_demo`を事前に1回試して詰まりどころを把握しておく
- YAMLファイルのインデントミスで`--params-file`が無視される事故が起きやすいので、講師の手元にも動作確認済みのYAMLを用意しておく

### 時間配分の目安

- パラメータ変更コールバックの説明とコード実装: 15分
- YAMLファイルの作成とsetup.pyへのinstall設定: 10分
- launchファイルの作成（Node/DeclareLaunchArgument）: 15分
- 課題（YAML変更→再起動、param setでの変更確認）: 15分
- 口頭試問: 10分

### 口頭試問

**Q1. `declare_parameter`だけを実装し、`add_on_set_parameters_callback`を実装していないノードに対して`ros2 param set`を叩くと何が起きますか。**

模範解答: パラメータサーバー上の値は書き換わり、`ros2 param get`で確認すると新しい値が返ってくる。しかしノードの内部変数（コンストラクタで`get_parameter`した時点の値を保持している変数）は自動更新されないため、実際の処理には反映されない。値の「表示上の変更」と「動作への反映」は別物であり、後者を実現するにはコールバックで明示的に内部変数を更新する処理が必要。

**Q2. launchファイルの`Node`に`parameters=['config/speed_param.yaml']`と直接書くのと、`setup.py`でYAMLをinstallしてから`os.path.join(get_package_share_directory(...), 'config', 'speed_param.yaml')`のパスを渡すのとで、何が違いますか。**

模範解答: 相対パスをそのまま書く方法は、`ros2 launch`をどのディレクトリから実行するかに動作が依存し、別の作業ディレクトリから起動すると失敗する。`get_package_share_directory`で取得したインストール先のパスを使う方法は、`colcon build`でパッケージがインストールされたあとであれば実行場所に関係なく同じYAMLを見つけられる。配布・共同利用を考えるなら後者が正しい書き方であり、YAMLを`setup.py`の`data_files`できちんとinstall対象に含めておく必要がある。

**Q3. テストラン現場で`max_speed`を調整したい場面を想定してください。ソースコードの`declare_parameter('max_speed', 1.0)`のデフォルト値を直接書き換えて再ビルドする方法と、launchが読み込むYAMLファイルの値だけを書き換えて再起動する方法を比べて、どちらが現場向きですか。**

模範解答: YAMLファイルの値だけを書き換える方法が現場向き。前者はソース変更→`colcon build`→インストール反映という手順が挟まり、ビルドに数十秒〜数分かかることも珍しくない。後者はテキストエディタでYAMLの数値を直して`ros2 launch`をもう一度叩くだけで済む。テストラン現場ではこの数分の差が積み重なって全体の調整回数を減らすため、パラメータをコードに埋め込まずYAML経由にしておく設計が望ましい。

## 本文

### 学習内容：パラメータを持つノードを書く

学習内容：`declare_parameter`でパラメータを宣言し、`get_parameter`で読み出す。

準備：ワークスペース内にPythonパッケージ`speed_param_demo`を作成しておきます。

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_python speed_param_demo
```

内容：

`speed_param_demo/speed_param_demo/speed_node.py`を次の内容で作成します。`max_speed`というパラメータを持ち、タイマーで一定周期ごとに現在の値をログ出力するだけの単純なノードです。

```python
import rclpy
from rclpy.node import Node


class SpeedNode(Node):
    def __init__(self):
        super().__init__('speed_node')

        # デフォルト値1.0で宣言する。宣言していないパラメータは
        # get_parameterで例外になるため、使うパラメータは必ず宣言する。
        self.declare_parameter('max_speed', 1.0)

        # コンストラクタ時点の値を内部変数に保持しておく。
        self.max_speed = self.get_parameter('max_speed').get_parameter_value().double_value

        self.create_timer(1.0, self.on_timer)

    def on_timer(self):
        self.get_logger().info(f'max_speed = {self.max_speed}')


def main():
    rclpy.init()
    node = SpeedNode()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

ヒント：`get_parameter('max_speed').get_parameter_value().double_value`は少し長く見えますが、パラメータの型（int/double/string/boolなど）を明示的に指定する書き方です。型を意識せず`.value`だけで済ませるコード例も見かけますが、意図しない型変換に気づきにくくなるため、型を明示する書き方に慣れておいてください。

`setup.py`の`entry_points`に登録し、ビルド後に単体で起動できることを確認します。

```bash
colcon build --packages-select speed_param_demo
source install/setup.bash
ros2 run speed_param_demo speed_node
```

1秒ごとに`max_speed = 1.0`とログが出れば成功です。別ターミナルで`ros2 param set /speed_node max_speed 5.0`を叩いてみてください。`ros2 param get /speed_node max_speed`は5.0を返しますが、ログの表示は1.0のまま変わりません。これが[07_パラメータ](07_パラメータ.md)で触れた「setしても効かない」現象です。

### 学習内容：パラメータ変更コールバックを実装する

学習内容：`add_on_set_parameters_callback`で実行中の変更を受け取り、内部変数を更新する。

準備：課題1のノードが起動できる状態であること。

内容：

`speed_node.py`のコンストラクタにコールバック登録を追加します。

```python
import rclpy
from rclpy.node import Node
from rcl_interfaces.msg import SetParametersResult


class SpeedNode(Node):
    def __init__(self):
        super().__init__('speed_node')

        self.declare_parameter('max_speed', 1.0)
        self.max_speed = self.get_parameter('max_speed').get_parameter_value().double_value

        # パラメータ変更のたびにon_parameter_changeが呼ばれるようになる。
        self.add_on_set_parameters_callback(self.on_parameter_change)

        self.create_timer(1.0, self.on_timer)

    def on_parameter_change(self, params):
        for param in params:
            if param.name == 'max_speed':
                # 負の速度は認めない、というような検証もここに書ける。
                if param.value < 0.0:
                    return SetParametersResult(successful=False, reason='max_speed must be >= 0')
                self.max_speed = param.value
        return SetParametersResult(successful=True)

    def on_timer(self):
        self.get_logger().info(f'max_speed = {self.max_speed}')


def main():
    rclpy.init()
    node = SpeedNode()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

`add_on_set_parameters_callback`に登録した関数は、`ros2 param set`が呼ばれるたびに変更対象のパラメータ一覧（`params`）を受け取って実行されます。`SetParametersResult(successful=True)`を返すと変更が確定し、`successful=False`を返すと変更が拒否されて元の値のまま残ります。ここでは`max_speed`が負の値になろうとしたら拒否する検証も入れています。パラメータサーバー上の値を書き換えるだけでなく、実際にロボットの挙動を左右する変数（この例では`self.max_speed`）を更新する処理を必ずセットで書く、というのがこの課題の核心です。

再ビルドして動かします。

```bash
colcon build --packages-select speed_param_demo
source install/setup.bash
ros2 run speed_param_demo speed_node
```

別ターミナルから値を変えてみます。

```bash
ros2 param set /speed_node max_speed 5.0
```

今度はログが`max_speed = 5.0`に切り替わります。さらに負の値を試してください。

```bash
ros2 param set /speed_node max_speed -1.0
```

コマンドは失敗し、`ros2 param get /speed_node max_speed`で確認しても値は5.0のまま変わっていません。バリデーションが機能している証拠です。

> コラム: なぜバリデーションをコールバック側に書くのか
>
> `max_speed`のような値は、モータドライバの仕様や機体の物理的な限界で下限・上限が決まっています。範囲外の値がそのまま内部変数に入ってしまうと、テストラン現場での一瞬のタイポで機体が暴走する可能性があります。コールバックで拒否できるようにしておけば、`ros2 param set`を打つ側が数値を打ち間違えても機体側で弾かれます。ロボコンの機体ではこの検証を「あれば安心」ではなく「必須」として扱ってください。

### 学習内容：パラメータYAMLをパッケージにinstallする

学習内容：`setup.py`の`data_files`にYAMLを追加し、`get_package_share_directory`で読み込めるようにする。

準備：課題2のパッケージ内に`config`ディレクトリを作ります。

```bash
cd ~/ros2_ws/src/speed_param_demo
mkdir config
```

内容：

`config/speed_param.yaml`を作成します。ノード名をキーにした構造で、`ros`名前空間の下に`node_name`、その下に`ros__parameters`というキー構造が固定です。

```yaml
speed_node:
  ros__parameters:
    max_speed: 2.0
```

`setup.py`を編集し、`config`ディレクトリの中身がインストール先に配置されるように`data_files`へ追加します。

```python
import os
from glob import glob
from setuptools import setup

package_name = 'speed_param_demo'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        # launchファイルとYAMLファイルをshare/以下にコピーする指定
        (os.path.join('share', package_name, 'launch'), glob('launch/*.py')),
        (os.path.join('share', package_name, 'config'), glob('config/*.yaml')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    entry_points={
        'console_scripts': [
            'speed_node = speed_param_demo.speed_node:main',
        ],
    },
)
```

`glob('config/*.yaml')`のように書いておくと、YAMLファイルを追加・削除してもリストを毎回書き換える必要がありません。`launch`ディレクトリも同じ要領で追加してあります（次の課題ですぐ使います）。

ビルドし、YAMLが本当にインストール先へコピーされたか確認します。

```bash
colcon build --packages-select speed_param_demo
ls install/speed_param_demo/share/speed_param_demo/config/
```

`speed_param.yaml`が表示されればinstallの設定は成功です。ヒント：ここで表示されない場合、`setup.py`の`data_files`の書き方がまず疑わしいです。`colcon build`はエラーを出さずに単に対象ファイルをコピーし忘れることがあるので、必ず`ls`で目視確認する癖をつけてください。

`--params-file`で直接指定して起動できることも確認します。

```bash
ros2 run speed_param_demo speed_node --ros-args --params-file install/speed_param_demo/share/speed_param_demo/config/speed_param.yaml
```

`max_speed = 2.0`とログが出れば、YAMLの値が正しく読み込まれています。

### 学習内容：launchファイルからYAMLを注入する

学習内容：`Node`アクションの`parameters=`引数でYAMLファイルを渡す。

準備：課題3のYAMLがインストール済みであること。

内容：

`launch/speed_param_launch.py`を作成します。

```python
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory('speed_param_demo'),
        'config',
        'speed_param.yaml',
    )

    return LaunchDescription([
        Node(
            package='speed_param_demo',
            executable='speed_node',
            name='speed_node',
            parameters=[config],
        ),
    ])
```

`get_package_share_directory('speed_param_demo')`は、colconがインストールしたこのパッケージのshareディレクトリの絶対パスを返します。これに`config/speed_param.yaml`を連結することで、`ros2 launch`をどのディレクトリから実行しても同じYAMLファイルを指すパスが組み立てられます。`Node`の`parameters=`にはYAMLファイルのパスの他、辞書（`{'max_speed': 2.0}`）を直接渡すこともできますが、テストラン現場での書き換えやすさを考えるとYAML経由に統一しておくほうが扱いやすいです。

`setup.py`にはすでに`launch`ディレクトリのinstall設定を入れてあるので、再ビルドして実行します。

```bash
colcon build --packages-select speed_param_demo
source install/setup.bash
ros2 launch speed_param_demo speed_param_launch.py
```

`max_speed = 2.0`とログが出れば、YAML経由での注入に成功しています。ここで一度Ctrl+CしてYAMLの`max_speed`の値を`8.0`に書き換えてから、もう一度同じ`ros2 launch`コマンドを叩いてください。ログが`max_speed = 8.0`に変わることを確認します。ソースコードは1文字も変えずに済んでいます。

### 学習内容：launch引数で起動時の値を切り替える

学習内容：`DeclareLaunchArgument`と`LaunchConfiguration`で、コマンドライン引数からremappingや名前空間を渡せるようにする。

準備：課題4のlaunchファイルが動作すること。

内容：

複数台の機体を同じlaunchファイルで起動したい場面を想定し、`namespace`をコマンドラインから指定できるようにします。

```python
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory('speed_param_demo'),
        'config',
        'speed_param.yaml',
    )

    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='ノードの名前空間。複数台の機体を区別するときに使う',
    )

    return LaunchDescription([
        namespace_arg,
        Node(
            package='speed_param_demo',
            executable='speed_node',
            name='speed_node',
            namespace=LaunchConfiguration('namespace'),
            parameters=[config],
            remappings=[
                ('/cmd_vel', 'cmd_vel'),
            ],
        ),
    ])
```

`DeclareLaunchArgument('namespace', default_value='')`は、`ros2 launch`実行時に`namespace:=値`という形式で渡せる引数を定義しています。`LaunchConfiguration('namespace')`はその値を実行時に解決するプレースホルダで、`Node`の`namespace=`に渡すと起動時に確定します。デフォルト値を空文字にしておくことで、引数を省略した場合は名前空間なしで起動する挙動になります。

**ここで一度必ず踏むワナがあります。** 上で作った`speed_param.yaml`のトップレベルキーは`speed_node:`（名前空間なし）ですが、`namespace:=robot1`を付けて起動するとノードの完全修飾名は`/robot1/speed_node`になります。パラメータYAMLは名前空間まで含めてノード名を照合するため、`speed_node:`というキーは`/robot1/speed_node`に**マッチしません**。エラーにはならず、`declare_parameter`のデフォルト値のまま起動します。

```
# namespace なし
[speed_node-1] [INFO] [speed_node]: max_speed = 2.0     ← YAML が効いている
# namespace:=robot1
[speed_node-1] [INFO] [robot1.speed_node]: max_speed = 1.0   ← デフォルト値のまま
```

複数台の機体で同じYAMLを使い回したい場合は、トップレベルキーを`/**`のワイルドカードにします。こうすると名前空間が付いても適用されます。

```yaml
/**:
  ros__parameters:
    max_speed: 2.0
```

「YAMLを渡したのに値が変わらない」というときは、まず`ros2 param get /<名前空間>/<ノード名> <パラメータ名>`で実際の値を確認し、次にYAMLのキーが完全修飾名と一致しているかを見てください。

引数なしで起動すると今まで通りです。

```bash
ros2 launch speed_param_demo speed_param_launch.py
```

引数を付けて起動すると、ノード名が`/robot1/speed_node`のように名前空間付きになります。

```bash
ros2 launch speed_param_demo speed_param_launch.py namespace:=robot1
```

別ターミナルで`ros2 node list`を叩き、`/robot1/speed_node`が見えることを確認してください。

**練習問題**: このlaunchファイルに、YAMLファイルのパスそのものをlaunch引数で切り替えられるようにする変更を加えてください。デフォルトはこれまでの`config/speed_param.yaml`のままとし、`params_file:=`で別のYAMLパスを指定できるようにします。

<details markdown="1"><summary>解答</summary>

```python
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    default_config = os.path.join(
        get_package_share_directory('speed_param_demo'),
        'config',
        'speed_param.yaml',
    )

    params_file_arg = DeclareLaunchArgument(
        'params_file',
        default_value=default_config,
        description='読み込むパラメータYAMLファイルのパス',
    )

    return LaunchDescription([
        params_file_arg,
        Node(
            package='speed_param_demo',
            executable='speed_node',
            name='speed_node',
            parameters=[LaunchConfiguration('params_file')],
        ),
    ])
```

`default_value`にデフォルトのYAMLパスを渡し、`parameters=[LaunchConfiguration('params_file')]`とすることで、引数を省略すればいつも通り、指定すれば別のYAML（例えば`params_file:=/home/user/tuned_speed_param.yaml`）を読み込む挙動になります。テストラン現場で複数の調整パターンをYAMLファイルとして残しておき、状況に応じて切り替えたい場合にこの形が使えます。

</details>

## 発展

今回のノードは`max_speed`という単一の値でしたが、実際のロボコン機体では制御ゲイン（Kp/Ki/Kd）、機体寸法、フレームIDなど数十個のパラメータを扱うことになります。パラメータが増えると、YAML1つに全ノードの設定をまとめて、複数のlaunchファイルから同じYAMLの一部だけを読ませたくなる場面が出てきます。この場合はYAML側でノード名をワイルドカード（`/**`）にする書き方や、`launch.actions.GroupAction`でノード群をまとめて扱う方法があります。


Pythonのパラメータ型は暗黙のリスト（`declare_parameter('gains', [1.0, 0.1, 0.01])`のような配列パラメータ）も扱えますが、型指定を誤ると起動時エラーになりやすい部分です。配列パラメータを本格的に使う段階になったら公式ドキュメントの型の章を読み直してください。

## おわりに

パラメータとlaunchを組み合わせられるようになると、テストラン現場での調整はソースコードの再ビルドから解放されます。「launchのYAMLだけ直して再起動」で挙動が変わる構成が理想であり、逆にパラメータをコードにハードコードしたまま現場に持ち込むと、その場での微調整のたびにビルド待ちが発生します。今回作った`speed_node`のように、調整したい値は必ず`declare_parameter`で宣言し、実行中に効かせたい値は必ず`add_on_set_parameters_callback`で内部変数に反映する、という2点をセットで覚えておいてください。

次は[16_アクションサーバの実装](16_アクションサーバの実装.md)で、feedback付きの長時間タスクをサーバ側から実装します。わからないところがあれば先輩に聞きましょう。

### 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `06_parameters` — クラスの中でパラメータを使う
- `07_param_events` — パラメータの変更を監視する
- `08_params_yaml` — パラメータを YAML で管理する
- `09_launch` — launch を Python / XML / YAML で書く

```bash
./drill run 06
./drill run 07
./drill run 08
./drill run 09
```

課題側からは `./drill read` でこの章に戻ってこられます。

## 資料

- [Using parameters in a class (Python) — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Using-Parameters-In-A-Class-Python.html)
- [Launch — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Launch/Launch-Main.html)
- [07_パラメータ](07_パラメータ.md)
- [09_launchとros2_bag](09_launchとros2_bag.md)
- 前回: [14_サービスの実装](14_サービスの実装.md)
- 次回: [16_アクションサーバの実装](16_アクションサーバの実装.md)
