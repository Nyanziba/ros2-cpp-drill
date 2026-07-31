# ROS2講習11: C++でpub/subを書く

## はじめに

この記事を終えると、C++で自作のpublisherノードとsubscriberノードを書き、ビルドして実際に通信させられるようになります。

前提は[10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)を読み終えていることです。ワークスペースの作り方とcolcon buildの流れがわからないと、この記事のパッケージ作成でつまずきます。

## 講習目標

- `ros2 pkg create`でC++パッケージを作成できる
- `rclcpp::Node`を継承したpublisherノードとsubscriberノードを書ける
- `package.xml`と`CMakeLists.txt`に依存関係を正しく追記できる
- ビルドしたノードを2ターミナルで動かし、`ros2 topic echo`で通信を確認できる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jaliscoがセットアップ済みの環境（[02_環境構築](02_環境構築.md)完了）
- ワークスペース（例: `~/ros2_ws`）が作成済みで、`colcon build`が一度は通っていること（[10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)完了）
- エディタ（VS Code等）でC++の構文ハイライトが効く状態
- ターミナルを3枚（ビルド用、publisher実行用、subscriber実行用）並べて開ける画面

### 時間配分の目安

- パッケージ作成とディレクトリ構成の説明: 10分
- publisher/subscriberコード読解: 20分
- CMakeLists.txt/package.xml編集とビルド: 15分
- 実行・echo確認・詰まりポイントの解消: 15分
- 口頭試問: 10分

### 口頭試問

**Q1. `create_wall_timer`で渡したコールバック関数は何を基準に呼ばれますか。トピックの受信とは何が違いますか。**

模範解答: `create_wall_timer`は壁時計（wall clock）の経過時間を基準に、指定した周期で自動的にコールバックを呼び出す。これはトピックの受信とは無関係で、メッセージが来ても来なくても一定間隔で発火する。一方subscriberのコールバックはメッセージが実際にトピックへpublishされたタイミングで呼ばれる、イベント駆動です。publisherのループ処理をタイマー任せにできるのがROS2らしい書き方で、`while(true)`のような自前ループを書く必要がありません。

**Q2. `rclcpp::spin(node)`を呼ばないとどうなりますか。**

模範解答: `spin`はノードのイベントループを回す関数で、タイマーやsubscriberのコールバックを実際に呼び出す役目を持つ。`spin`を呼ばないとノードは起動してすぐプログラムが終了してしまい、コールバックが一度も実行されない。publisherもsubscriberも、コンストラクタでタイマーやsubscriberを登録するだけでは何も起きず、`spin`が待ち受けてこそ動き出す。

**Q3. CMakeLists.txtに`ament_target_dependencies`を書き忘れるとどんなエラーになりますか。**

模範解答: コンパイル自体は`#include`が通れば進むことがあるが、リンク時に`undefined reference to rclcpp::...`のようなリンカエラーが出ることが多い。`ament_target_dependencies`は該当パッケージ（`rclcpp`や`std_msgs`）のインクルードパスとライブラリをターゲットに紐付ける役目なので、書き忘れるとヘッダは見つかってもシンボルが解決できない。逆に`package.xml`側の依存宣言を忘れると、他の環境でこのパッケージをビルドしようとしたときに依存パッケージが自動で入らず、colconのビルド順序解決にも影響する。

## 本文

### 学習内容：パッケージを作る

学習内容：`ros2 pkg create`でC++用のパッケージを新規作成する。

準備：ワークスペースの`src`ディレクトリに移動しておきます。

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake --license Apache-2.0 cpp_pubsub
```

内容：

`--build-type ament_cmake`はC++パッケージであることを指定するオプションです。Pythonパッケージなら`ament_python`を使いますが、これは[12_Pythonでpub_subを書く](12_Pythonでpub_subを書く.md)で扱います。実行すると以下の構成が生成されます。

```
cpp_pubsub/
├── CMakeLists.txt
├── include/
│   └── cpp_pubsub/
├── package.xml
└── src/
```

`src/`にソースファイルを置き、ビルド設定は`CMakeLists.txt`、パッケージのメタ情報と依存関係は`package.xml`に書きます。この2ファイルを正しく編集しないとどれだけコードが正しくてもビルドが通らないので、コードを書く前に構成だけ頭に入れておいてください。

### 学習内容：publisherノードを書く

学習内容：一定周期でメッセージをpublishするノードを、公式チュートリアルのコードに沿って書く。

準備：`cpp_pubsub/src/publisher_member_function.cpp`を新規作成します。

内容：

以下は[公式チュートリアル](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)のpublisherコードに日本語コメントを付けたものです。まずは全文を掲載します。

```cpp
#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

// std::chronoのリテラル（500msなど）を使えるようにする
using namespace std::chrono_literals;

// rclcpp::Nodeを継承して独自のノードクラスを作る
class MinimalPublisher : public rclcpp::Node
{
public:
  MinimalPublisher()
  : Node("minimal_publisher"), count_(0)
  {
    // "topic"という名前のトピックにString型メッセージをpublishする
    // 第2引数の10はQoSの depth（送信キューの深さ）
    publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);

    // 500ms周期でtimer_callbackを呼び出すタイマーを作成する
    timer_ = this->create_wall_timer(
      500ms, std::bind(&MinimalPublisher::timer_callback, this));
  }

private:
  void timer_callback()
  {
    auto message = std_msgs::msg::String();
    message.data = "Hello, world! " + std::to_string(count_++);
    RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
    publisher_->publish(message);
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  size_t count_;
};

int main(int argc, char * argv[])
{
  // rclcppの初期化（ノードを作る前に必ず呼ぶ）
  rclcpp::init(argc, argv);

  // ノードを生成してspinに渡す。spinがコールバックを実際に呼び続ける
  rclcpp::spin(std::make_shared<MinimalPublisher>());

  // Ctrl+Cなどで抜けたら終了処理
  rclcpp::shutdown();
  return 0;
}
```

要点は3つです。

- **タイマーコールバック駆動**: このpublisherは何かのイベントを待つのではなく、`create_wall_timer`で登録した500ms周期のタイマーによって`timer_callback`が定期的に呼ばれる仕組みです。ループを自分で書く必要はなく、周期はコンストラクタで一度指定するだけです。
- **QoS depth 10**: `create_publisher`の第2引数`10`は送信側のキューにいくつメッセージを保持するかを指定するQoS設定です。subscriberの受信処理が一時的に遅れても、直近10件分はバッファされて失われにくくなります。深いQoSの話は[05_トピック](05_トピック.md)を参照してください。
- **`std::bind`**: `std::bind(&MinimalPublisher::timer_callback, this)`はメンバ関数`timer_callback`を、`this`（現在のインスタンス）に束縛した呼び出し可能オブジェクトに変換しています。ラムダで`[this]() { this->timer_callback(); }`と書いても同じ意味です。Jazzyの公式チュートリアルが読者に書かせるのは実はラムダ版（`publisher_lambda_function.cpp` / `subscriber_lambda_function.cpp`）で、ここで使っている`std::bind`のメンバ関数版は`ros2/examples`リポジトリに別バリアントとして併記されているものです。ROS 2のスタイルガイド（[Code style and language versions](https://docs.ros.org/en/jazzy/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html)）は「ラムダ・`std::function`・`std::bind`に制限なし」と明記しているので、どちらを使っても構いません。ちなみに公式が明確に非推奨としているのは書き方ではなく「`rclcpp::Node`を継承しない書き方」（componentにできないため）です。

### 学習内容：subscriberノードを書く

学習内容：トピックを受信してログに出すノードを書く。

準備：`cpp_pubsub/src/subscriber_member_function.cpp`を新規作成します。

内容：

```cpp
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using std::placeholders::_1;

class MinimalSubscriber : public rclcpp::Node
{
public:
  MinimalSubscriber()
  : Node("minimal_subscriber")
  {
    // "topic"という名前のトピックをsubscribeする
    // 第2引数の10はpublisher側と同じくQoSのdepth
    subscription_ = this->create_subscription<std_msgs::msg::String>(
      "topic", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));
  }

private:
  void topic_callback(const std_msgs::msg::String & msg) const
  {
    RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg.data.c_str());
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MinimalSubscriber>());
  rclcpp::shutdown();
  return 0;
}
```

publisherとの対称性に注目してください。`create_publisher`が`create_subscription`に変わり、タイマーの代わりにコールバック関数`topic_callback`を直接渡しています。`std::bind`の`_1`はメッセージが渡ってくる引数の位置を表すプレースホルダで、実際にメッセージが届くたびに`topic_callback(msg)`が呼ばれます。`topic_callback`が`const`メンバ関数になっているのは、受信したメッセージをログに出すだけでノード自身の状態を変更しないためです。

subscriberのトピック名`"topic"`とQoS depth`10`はpublisher側と一致させる必要があります。片方だけ変えると通信が繋がらなくなるので注意してください。

### 学習内容：CMakeLists.txtとpackage.xmlを編集する

学習内容：ビルドに必要な依存関係と実行ファイルの設定を書く。

準備：`cpp_pubsub/CMakeLists.txt`と`cpp_pubsub/package.xml`を開きます。

内容：

まず`package.xml`に依存パッケージを追記します。`<description>`の下あたり、既存の`<depend>`タグが並ぶ場所に以下を加えてください。

```xml
<depend>rclcpp</depend>
<depend>std_msgs</depend>
```

次に`CMakeLists.txt`です。デフォルトで生成される`find_package(ament_cmake REQUIRED)`の下に、使うパッケージの`find_package`を追加します。

```cmake
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
```

そのあとに実行ファイルの定義を追加します。ノードごとに`add_executable`と`ament_target_dependencies`のペアが必要です。

```cmake
add_executable(talker src/publisher_member_function.cpp)
ament_target_dependencies(talker rclcpp std_msgs)

add_executable(listener src/subscriber_member_function.cpp)
ament_target_dependencies(listener rclcpp std_msgs)

install(TARGETS
  talker
  listener
  DESTINATION lib/${PROJECT_NAME})
```

`add_executable`は「このソースファイルからこの名前の実行ファイルを作る」という指定、`ament_target_dependencies`は「このターゲットにこのパッケージのヘッダとライブラリを紐付ける」という指定です。`install`を書かないと`colcon build`は通ってもインストール先（`ros2_ws/install/`以下）に実行ファイルが配置されず、`ros2 run`で見つからないという事態になります。

### 学習内容：ビルドして実行する

学習内容：`colcon build`でビルドし、2ターミナルでpublisherとsubscriberを動かす。

準備：ワークスペースのルートに移動します。

内容：

```bash
cd ~/ros2_ws
colcon build --packages-select cpp_pubsub
source install/setup.bash
```

`--packages-select`で対象を絞るとビルドが速くなります。ビルドが通ったら、ターミナルを2枚用意してそれぞれ`source install/setup.bash`してから実行します。

ターミナル1（publisher）:

```bash
ros2 run cpp_pubsub talker
```

ターミナル2（subscriber）:

```bash
ros2 run cpp_pubsub listener
```

talker側に`Publishing: 'Hello, world! 0'`のようなログが500ms間隔で出て、listener側に`I heard: 'Hello, world! 0'`が対応して出てくれば成功です。3枚目のターミナルから`ros2 topic echo /topic`を実行しても同じデータが流れているのが確認できます。

**練習問題**: talkerを起動したまま、listenerを後から起動しても正しく通信できることを確認してください。逆にlistenerを先に起動した場合はどうなるか予想してから試してください。

<details markdown="1"><summary>解答</summary>

どちらの順番でも通信できます。ROS2のトピックはDDSのdiscovery機能により、ノードが起動した順序に関係なく相手を見つけてつながる仕組みになっているためです（discoveryの詳細は[05_トピック](05_トピック.md)の発展節を参照）。ただしlistenerを後から起動した場合、起動前にpublishされたメッセージは受け取れません。QoS depthのバッファはpublisher/subscriberが繋がった状態を前提にした短期的な取りこぼし対策であり、繋がっていない間のメッセージまでは遡って届けてくれません。

</details>

### 詰まりポイント

- **CMakeLists.txtの編集忘れ**: ソースファイルを追加しても`add_executable`を書かないと`colcon build`は何も問題なく通ってしまいます（新しいノードの存在を知らないだけ）。`ros2 run cpp_pubsub talker`で`Package 'cpp_pubsub' not found`ではなく`No executable found`系のエラーが出たら、まずCMakeLists.txtの`add_executable`と`install`を確認してください。
- **依存追加漏れ**: `package.xml`に`<depend>rclcpp</depend>`を書き忘れると、単体でビルドしている間は気づきにくいですが、`rosdep`で依存解決する場面やCIでエラーになります。CMakeLists.txt側だけ`find_package`していても`package.xml`との不一致は`colcon build`で警告が出ることがあるので、警告を無視しないでください。
- **`ament_target_dependencies`の書き忘れ**: ビルドエラーの出方は`undefined reference to `rclcpp::...``のようなリンカエラーです。「コンパイルは通ったのにリンクで落ちる」ときはまずこれを疑ってください。ヘッダの`#include`とライブラリのリンクは別の設定だという点を覚えておくと、この種のエラーで慌てなくなります。
- **`source install/setup.bash`を忘れる**: ビルドし直した直後の新しいターミナルでは、そのターミナルにまだ新しい実行ファイルの場所が反映されていません。ノードが変更したはずなのに古い挙動をする場合は、まず`source`を忘れていないか確認してください。

## 発展

talkerとlistenerが動いたら、既存のノードを改造して`cmd_vel`をsubscribeし、中身をログに出すノードを書いてみてください。`geometry_msgs/msg/Twist`型のトピックを受け取り、`linear.x`と`angular.z`を`RCLCPP_INFO`で表示するだけの改造です。`std_msgs::msg::String`を`geometry_msgs::msg::Twist`に差し替え、`package.xml`と`CMakeLists.txt`に`geometry_msgs`の依存を追加する必要があります。[05_トピック](05_トピック.md)の`turtlesim`と組み合わせて、`turtle_teleop_key`で動かした亀の速度指令を自作ノードで受け取れれば成功です。

## おわりに

C++でのpub/subはROS2のノードを書く上でいちばん基本の型です。ここで書いたtalker/listenerの構造（Node継承、create_publisher/create_subscription、コンストラクタでの登録、spinでの実行）は今後どんなノードを書いても繰り返し出てきます。わからなければ先輩に聞きましょう。

次は[12_Pythonでpub_subを書く](12_Pythonでpub_subを書く.md)で、同じ内容をPythonで書きます。C++とどこが違ってどこが同じか、書き比べてみてください。

### 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `01_publisher` — トピックに publish する
- `02_subscriber` — トピックを購読する

```bash
./drill run 01
./drill run 02
```

課題側からは `./drill read` でこの章に戻ってこられます。

## 資料

- [ROS 2 Documentation: Jazzy — Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
- [10_ワークスペースとcolcon](10_ワークスペースとcolcon.md)
- [12_Pythonでpub_subを書く](12_Pythonでpub_subを書く.md)
