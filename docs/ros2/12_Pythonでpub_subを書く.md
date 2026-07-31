# ROS2講習12: Pythonでpub/subを書く

## はじめに

この記事を終えると、`rclpy`を使ってPythonでpublisherとsubscriberを自作し、ビルドして実行できるようになります。あわせて、[11_C++でpub_subを書く](11_C++でpub_subを書く.md)で作ったC++ノードとPythonノードを同じトピックで繋ぎ、言語が違っても通信できることを確認します。

前提は[11_C++でpub_subを書く](11_C++でpub_subを書く.md)と[05_トピック](05_トピック.md)を読み終えていることです。C++版と作るものはほぼ同じなので、この記事はC++版との違いに絞って説明します。

## 講習目標

- `ros2 pkg create --build-type ament_python`でPythonパッケージを作れる
- `rclpy`の`Node`を継承したpublisher/subscriberを書ける
- `setup.py`の`entry_points`を編集し、`ros2 run`で実行できる状態にできる
- `ament_python`パッケージでも`colcon build`が必要な理由を説明できる
- C++ノードとPythonノードをトピックで相互通信させ、言語の違いを気にせず使い分けられる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jaliscoがセットアップ済みの環境（[02_環境構築](02_環境構築.md)完了）
- [11_C++でpub_subを書く](11_C++でpub_subを書く.md)で作った`cpp_pubsub`パッケージ（`talker`/`listener`）がビルド済みであること
- ワークスペース`~/ros2_ws`が存在し、`src`以下にパッケージを置ける状態であること
- ターミナルを3枚程度並べて開ける画面

### 時間配分の目安

- パッケージ作成とディレクトリ構成の説明: 10分
- publisher/subscriberのコード解説: 20分
- ビルド・実行・詰まりポイントの確認: 15分
- C++ノードとの相互通信課題: 15分
- 口頭試問: 10分

### 口頭試問

**Q1. `ament_python`ビルドタイプのパッケージなのに、なぜ`colcon build`が必要なのですか。Pythonはインタプリタ言語だから直接実行できるはずでは。**

模範解答: `colcon build`はコンパイルのためだけの仕組みではない。`ament_python`パッケージでも、`setup.py`の`entry_points`から実行スクリプトを生成し、`install`ディレクトリ以下にパッケージを配置し、`package.xml`の依存関係を解決してROS 2のパッケージ検索パス（`AMENT_PREFIX_PATH`）に登録する処理が必要になる。この登録がないと`ros2 run`はパッケージを見つけられない。`--symlink-install`を使えばソースの再コピーは省けるが、それでも初回の`colcon build`自体は必須。

**Q2. `setup.py`の`entry_points`を編集し忘れると何が起きますか。**

模範解答: `ros2 pkg create`が自動生成したノードファイルを書いても、`entry_points`の`console_scripts`にエントリを追加しない限り、`ros2 run パッケージ名 ノード名`で実行できる実行可能ファイルが生成されない。`colcon build`は成功するが、`ros2 run`時に「No executable found」のようなエラーになる。ノードを追加したら必ず`setup.py`も編集する癖をつける必要がある。

**Q3. C++で書くべき処理とPythonで書くべき処理を、制御周期の観点で説明してください。**

模範解答: モータ制御やセンサ処理のように数百Hz〜数kHzで動く必要がある処理、計算量が大きい画像処理・点群処理はC++で書くべき。Pythonはインタプリタのオーバーヘッドやグローバルインタプリタロック（GIL）の影響で高頻度・低遅延処理には向かない。一方、上位の状態遷移やタスク計画のような低頻度なロジック、デバッグ用のツール、実験用のスクリプト、機械学習ライブラリを使う処理はPythonのほうが書きやすく、開発速度が速い。両方をROS2のトピックで繋げば、それぞれの得意な言語で書ける。

## 本文

### 学習内容：Pythonパッケージを作る

学習内容：`ament_python`ビルドタイプでパッケージを作成する。

準備：ワークスペースの`src`ディレクトリに移動しておきます。

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_python py_pubsub --dependencies rclpy std_msgs
```

C++版では`ament_cmake`を指定して`CMakeLists.txt`が生成されましたが、今回は`ament_python`なので生成物が違います。

```
py_pubsub/
├── package.xml
├── setup.py
├── setup.cfg
├── resource/
│   └── py_pubsub
├── py_pubsub/
│   └── __init__.py
└── test/
```

注目すべきは`CMakeLists.txt`がなく、代わりに`setup.py`があることです。C++の`ament_cmake`パッケージはCMakeでビルドしますが、Pythonの`ament_python`パッケージはPythonの標準的なパッケージング機構（`setuptools`）に乗っています。ノードの実体を置くのは、パッケージ名と同じ`py_pubsub/py_pubsub/`ディレクトリの中です。

### 学習内容：publisherノードを書く

学習内容：`rclpy`の`Node`を継承したpublisherを書く。

準備：`py_pubsub/py_pubsub/`ディレクトリに`publisher_member_function.py`を作成します。

内容：

公式チュートリアル準拠のminimal publisherです。

```python
import rclpy
from rclpy.node import Node

from std_msgs.msg import String


class MinimalPublisher(Node):

    def __init__(self):
        super().__init__('minimal_publisher')
        self.publisher_ = self.create_publisher(String, 'topic', 10)
        timer_period = 0.5  # seconds
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.i = 0

    def timer_callback(self):
        msg = String()
        msg.data = 'Hello World: %d' % self.i
        self.publisher_.publish(msg)
        self.get_logger().info('Publishing: "%s"' % msg.data)
        self.i += 1


def main(args=None):
    rclpy.init(args=args)

    minimal_publisher = MinimalPublisher()

    rclpy.spin(minimal_publisher)

    # Destroy the node explicitly
    # (optional - otherwise it will be done automatically
    # when the garbage collector destroys the node object)
    minimal_publisher.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

C++版の`rclcpp::Node`と対応関係を見比べると理解が早いです。

| C++（rclcpp） | Python（rclpy） |
|---|---|
| `rclcpp::Node`を継承 | `rclpy.node.Node`を継承 |
| `this->create_publisher<T>(...)` | `self.create_publisher(T, ...)` |
| `this->create_wall_timer(...)` | `self.create_timer(...)` |
| `RCLCPP_INFO(this->get_logger(), ...)` | `self.get_logger().info(...)` |
| `rclcpp::spin(node)` | `rclpy.spin(node)` |

`create_timer`に渡している`timer_period`は秒単位のfloatで、C++版の`std::chrono::milliseconds(500)`と同じ0.5秒周期です。`self.i`はコールバックが呼ばれるたびに1増える単純なカウンタで、送るメッセージに埋め込んで「何回目のpublishか」がログで追えるようにしています。

`main`関数の最後で`destroy_node()`を呼んでいますが、コメントにある通りこれは必須ではありません。ガベージコレクタがノードオブジェクトを破棄する際に自動で呼ばれるので、書かなくても動きます。ただし公式チュートリアルが明示的に書いているのは、リソース解放のタイミングを自分で制御できることを示すためなので、そのまま残しておいて構いません。

### 学習内容：subscriberノードを書く

学習内容：`rclpy`の`Node`を継承したsubscriberを書く。

準備：同じディレクトリに`subscriber_member_function.py`を作成します。

内容：

```python
import rclpy
from rclpy.node import Node

from std_msgs.msg import String


class MinimalSubscriber(Node):

    def __init__(self):
        super().__init__('minimal_subscriber')
        self.subscription = self.create_subscription(
            String,
            'topic',
            self.listener_callback,
            10)
        self.subscription  # prevent unused variable warning

    def listener_callback(self, msg):
        self.get_logger().info('I heard: "%s"' % msg.data)


def main(args=None):
    rclpy.init(args=args)

    minimal_subscriber = MinimalSubscriber()

    rclpy.spin(minimal_subscriber)

    # Destroy the node explicitly
    # (optional - otherwise it will be done automatically
    # when the garbage collector destroys the node object)
    minimal_subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

`create_subscription`の第3引数`listener_callback`がメッセージ受信時に呼ばれる関数です。C++版では`std::bind`か`[this](...)`ラムダで束縛していましたが、Pythonはメンバ関数をそのまま渡すだけで、内部でインスタンスが束縛された状態のメソッドとして扱われます。`self.subscription  # prevent unused variable warning`の行はやや奇妙に見えますが、Pythonの静的解析ツールが「未使用変数」と誤検知するのを防ぐためのおまじないです。実際には`self.subscription`に代入した時点でノードのライフタイム中インスタンスが保持され続けるので、この行自体は動作に影響しません。

### 学習内容：setup.pyとpackage.xmlを編集する

学習内容：`entry_points`にノードを登録し、実行可能な状態にする。

準備：`setup.py`をエディタで開きます。

内容：

`setup.py`の中身は次のようになっています（自動生成された状態から編集が必要な箇所を示します）。

```python
from setuptools import find_packages, setup

package_name = 'py_pubsub'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='your_name',
    maintainer_email='you@example.com',
    description='Examples of minimal publisher/subscriber using rclpy',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'talker = py_pubsub.publisher_member_function:main',
            'listener = py_pubsub.subscriber_member_function:main',
        ],
    },
)
```

一番重要なのは`entry_points`の`console_scripts`です。`'talker = py_pubsub.publisher_member_function:main'`という書き方は「`ros2 run py_pubsub talker`と打ったら、`py_pubsub`パッケージの`publisher_member_function`モジュールの`main`関数を実行する」という意味になります。ここに書いた`talker`・`listener`という名前が、C++版で作った実行可能ファイル名（`talker`・`listener`）と同じであることに注意してください。この記事では意図的に同じ名前にしていますが、パッケージ名（`cpp_pubsub`/`py_pubsub`）が違うので実行時には区別できます。

`maintainer`と`maintainer_email`も自分の情報に書き換えておきます（空のままでもビルドは通りますが、警告が出ます）。

`package.xml`側は`ros2 pkg create`実行時に`--dependencies rclpy std_msgs`を指定したので、`exec_depend`が自動で入っています。

```xml
<exec_depend>rclpy</exec_depend>
<exec_depend>std_msgs</exec_depend>
```

C++版では`CMakeLists.txt`に`find_package`と`ament_target_dependencies`を書く必要がありましたが、Pythonの依存解決は`package.xml`の`exec_depend`のみで完結します。コンパイルが要らない分、依存の書き方はC++版よりシンプルです。

> コラム: `--dependencies`を付けずに`ros2 pkg create`した場合は、`package.xml`に`<exec_depend>`が入らないので、あとから手で追記する必要があります。依存を追加・削除したときに`package.xml`の更新を忘れるのは、C++・Python問わずROS2初心者が詰まりやすいポイントです。

### 学習内容：ビルドして実行する

学習内容：`colcon build`でビルドし、`ros2 run`で実行する。

準備：ワークスペースのルートに移動します。

```bash
cd ~/ros2_ws
```

内容：

```bash
colcon build --packages-select py_pubsub
```

ビルドが終わったら、新しいターミナルでオーバーレイを読み込んでから実行します。

```bash
source install/setup.bash
ros2 run py_pubsub talker
```

別のターミナルでも同様にオーバーレイを読み込んでからsubscriberを起動します。

```bash
source install/setup.bash
ros2 run py_pubsub listener
```

`talker`側で`Publishing: "Hello World: 0"`のようなログが0.5秒おきに出て、`listener`側で`I heard: "Hello World: 0"`が同じペースで出れば成功です。

**なぜ`ament_python`なのに`colcon build`が要るのか**

ここが今回の講習で一番誤解されやすいポイントです。「Pythonはコンパイル不要な言語なのだから、ファイルを書いたらすぐ`ros2 run`できるはず」と思いがちですが、実際には`colcon build`を通す必要があります。理由は次の3つです。

1. `setup.py`の`entry_points`から実行可能スクリプトを生成し、`install`ディレクトリに配置する必要がある
2. `package.xml`を解析して依存関係を検証し、ROS2のパッケージインデックス（`share/ament_index/resource_index/packages`）に登録する必要がある
3. `install/setup.bash`を読み込むことで、`AMENT_PREFIX_PATH`や`PYTHONPATH`にパッケージの場所が追加される

つまり`colcon build`はコンパイルのための工程ではなく、ROS2のパッケージ管理システムに登録するための工程です。C++と違ってビルド時間は一瞬ですが、工程自体は省略できません。

**詰まりポイント: `--symlink-install`**

ソースコードを1行直しただけなのに、いちいち`colcon build`し直すのは開発効率が悪いです。そこで`--symlink-install`オプションを使います。

```bash
colcon build --packages-select py_pubsub --symlink-install
```

このオプションを付けると、`install`ディレクトリにファイルをコピーする代わりにシンボリックリンクを張ります。Pythonはコンパイルが不要なので、ソースファイルを直接編集すればその変更がシンボリックリンク経由でそのまま反映され、再ビルドなしで`ros2 run`の結果に反映されます（`setup.py`の`entry_points`を変更した場合や新しいノードを追加した場合は、リンクの張り直しが必要になるので再ビルドしてください）。C++はコンパイルが必要なので、このオプションを使ってもソース変更後の再ビルドは省略できません。この違いを理解しておくと、Pythonノードの開発サイクルが一段速くなります。

ヒント：初回ビルド時から`--symlink-install`を付けておくと後で困りません。既に`--symlink-install`なしでビルドしてしまった場合は、一度`install`と`build`ディレクトリを消してから付け直すのが確実です。

### 学習内容：C++ノードとPythonノードを繋ぐ

学習内容：異なる言語で書かれたノードが同じトピックで通信できることを確認する。

準備：[11_C++でpub_subを書く](11_C++でpub_subを書く.md)の`cpp_pubsub`パッケージがビルド済みであることを確認します。

内容：

トピックは名前とメッセージ型だけで繋がる仕組みなので、送り手と受け手が同じ言語で書かれている必要はありません。実際に確かめてみましょう。

ターミナル1でC++のtalkerを起動します。

```bash
ros2 run cpp_pubsub talker
```

ターミナル2でPythonのlistenerを起動します。

```bash
ros2 run py_pubsub listener
```

C++のtalkerが送るメッセージをPythonのlistenerが受け取り、ログに表示されるはずです。逆方向、つまりPythonのtalkerとC++のlistenerの組み合わせでも同様に確認してください。

**練習問題**: ターミナル3を追加で開き、C++のtalker・Pythonのtalker・Pythonのlistenerを同時に起動してください。`listener`のログにはC++とPython、どちらのtalkerのメッセージも混ざって表示されるはずです。なぜそうなるのか、トピックの仕組みに基づいて説明してください。

<details markdown="1"><summary>解答</summary>

トピックは1対1の通信ではなく、同じトピック名・同じメッセージ型であれば複数のpublisherと複数のsubscriberが自由に繋がる多対多の仕組みです。C++のtalkerとPythonのtalkerはどちらも`topic`という名前の`std_msgs/msg/String`型トピックにpublishしているだけで、subscriber側からは送信元が何の言語で書かれたプロセスかを区別する情報がありません。そのため`listener`は両方のpublisherからのメッセージを区別なく受け取り、到着した順にコールバックが呼ばれます。ROS2のミドルウェア（DDS）はプロセス間通信の実体であり、実行バイナリの言語には関知しません。

</details>

このことから、ROS2では「どの言語で書くか」はノードの実装詳細であって、システム設計上はトピックのインターフェース（名前・型）さえ合わせれば自由に混在させられることがわかります。

**C++とPython、どちらで書くべきか**

実際にプロジェクトを組むときは、全部C++か全部Pythonかで統一する必要はありません。だいたいの判断基準は次の通りです。

| 処理の性質 | 向いている言語 | 理由 |
|---|---|---|
| モータ制御・センサドライバなど数百Hz以上で回る処理 | C++ | PythonのインタプリタオーバーヘッドとGILが低遅延処理のボトルネックになる |
| 画像処理・点群処理など計算量が大きい処理 | C++ | ネイティブコードの実行速度が必要 |
| 状態遷移・タスク計画などロジック中心で低頻度の処理 | Python | 開発速度が速く、書き換えの試行回数を増やせる |
| 可視化・デバッグ用ツール・ちょっとした実験スクリプト | Python | ライブラリが豊富で、その場で書いて捨てられる |

C++で全部書けないわけではないですが、「速度が要らないところをC++で書く」のはビルドの手間に対して見返りが薄いです。逆に「速度が必要なところをPythonで書く」のはGILの制約で詰みやすいです。両方の言語をトピックで繋げられるという今回確認した性質を活かして、処理内容に応じて言語を選び分けるのがROS2らしい設計だと言えます。

## 発展

`rclpy`にはC++の`rclcpp`と同じように、コールバックグループやマルチスレッドの`Executor`を使って複数のコールバックを並行実行する仕組みがあります。ただしPythonのGILの制約上、C++ほど並行処理の恩恵を受けやすいわけではありません。この点を深掘りするのはこの記事の範囲を超えるので、別記事に譲ります。


## おわりに

同じ`talker`/`listener`という構成でも、C++版とPython版でコード量も書き味もかなり違うことが実感できたはずです。どちらが優れているという話ではなく、処理内容に応じて使い分ける前提でROS2は設計されています。まずは両方書けるようになっておきましょう。わからなければ先輩に聞きましょう。

次は[13_カスタムインターフェース](13_カスタムインターフェース.md)で、`std_msgs/msg/String`のような既存の型ではなく、自分で定義したメッセージ型を使う方法を扱います。

## 資料

- [ROS 2 Documentation: Jazzy — Writing a simple publisher and subscriber (Python)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Publisher-And-Subscriber.html)
- [11_C++でpub_subを書く](11_C++でpub_subを書く.md)
- [05_トピック](05_トピック.md)
- [13_カスタムインターフェース](13_カスタムインターフェース.md)
