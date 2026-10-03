# ROS2講習15b: pluginlibで部品を設定から差し替える

## はじめに

この記事を終えると、`pluginlib`を使って「基底クラスを決め、実装をプラグインとして書き出し、YAMLに書いた名前で読み込む」仕組みを自分で作れるようになります。ROS 2の標準パッケージ（nav2やros2_controlなど）が「設定ファイルの`plugin:`に名前を書くだけで中身が変わる」のは、この仕組みのおかげです。

[04_ノード](04_ノード.md)の発展で、componentは「ノード」を同じプロセスに載せる仕組みだと触れました。pluginlibは、その1段内側、つまりノードの中の部品を差し替えるための仕組みです。パラメータとYAMLの扱いは[15_パラメータとlaunchの実践](15_パラメータとlaunchの実践.md)の続きとして読めます。

前提は、[11_C++でpub_subを書く](11_C++でpub_subを書く.md)までと[15_パラメータとlaunchの実践](15_パラメータとlaunchの実践.md)が済んでいて、C++のパッケージ（ament_cmake）をビルドできることです。基底クラスと継承（`virtual`）の知識も使います。

## 講習目標

- pluginlibが何をするか、いつ使うか（使わないほうがよいときも）を説明できる
- 基底クラス、プラグインのクラス、`PLUGINLIB_EXPORT_CLASS`、`plugins.xml`、CMakeとpackage.xmlの設定でプラグインを書き出せる
- `pluginlib::ClassLoader`で名前からプラグインを作り、失敗を`PluginlibException`で受けられる
- YAMLの`filters`の列と、プラグインごとの名前空間のパラメータで、プラグインの並びを組み立てられる

## 本文

### 1. pluginlibとは何か

pluginlibは、**クラスの名前（文字列）から、実行時にそのクラスのオブジェクトを作る**ためのライブラリです。部品は次の4つに分かれます。

- **基底クラス**: 差し替える部品が守る約束事（インターフェース）。使う側（ホスト）が決める
- **プラグイン**: 基底クラスを継承した実装。共有ライブラリ（`.so`）に入れて、`PLUGINLIB_EXPORT_CLASS`で登録する
- **`plugins.xml`**: 「この名前は、このライブラリのこのクラス」という対応表
- **`ClassLoader`**: ホストの中で、名前からプラグインを探して作る道具

ホストのコードはプラグインの実装を一切知りません（ヘッダも、リンクも要りません）。だから、**ホストのコードを書き換えず、再ビルドもせずに、設定の文字列だけで実装を差し替えられます**。別のパッケージで書かれた実装を後から足すこともできます。

いつ使うかをまとめると、次のとおりです。

- 使う: 実装を設定で選びたい。実装が増えていく。ホストを書く人と実装を書く人が別（別パッケージ、別チーム）
- 使わない: 実装が1つしかない。差し替えるのがコードを書く人だけで、再ビルドで困らない。この場合は普通の`virtual`クラスか関数で十分で、pluginlibは仕組みが増えるだけです

#### componentとの違い

[04_ノード](04_ノード.md)で出てきたcomponent（課題15）も、共有ライブラリを実行時に読み込みます。違いは「何を差し替えるか」です。

| | component（`rclcpp_components`） | pluginlib |
| --- | --- | --- |
| 差し替えるもの | **ノード**そのもの | **ノードの中の部品** |
| 基底クラス | `rclcpp::Node`で固定 | 自分で決める（この章では`VelocityFilter`） |
| 登録 | `RCLCPP_COMPONENTS_REGISTER_NODE` | `PLUGINLIB_EXPORT_CLASS` + `plugins.xml` |
| 読み込む側 | `component_container`（ROSが用意） | 自分のコードの`ClassLoader` |

componentは「どのノードを同じプロセスに載せるか」を選ぶ仕組みで、pluginlibは「ノードがどの部品を使うか」を選ぶ仕組みです。

### 2. 活用例

ROS 2の主要なパッケージは、どれもpluginlibで部品を差し替えています。「何が基底クラスで、どこで選ぶか」を並べます。

| 使う側 | 基底クラス（差し替える部品） | 選ぶ場所 |
| --- | --- | --- |
| nav2のplanner server | `nav2_core::GlobalPlanner` | YAMLの`planner_plugins`に名前の列、各名前の`plugin:`にクラス名 |
| nav2のcontroller server | `nav2_core::Controller` | YAMLの`controller_plugins`に名前の列、各名前の`plugin:`にクラス名 |
| nav2のcostmap | `nav2_costmap_2d::Layer` | YAMLの`plugins`に層の名前の列、各層の`plugin:`にクラス名 |
| ros2_control | コントローラは`ControllerInterface`、ハードウェアは`SystemInterface`など | コントローラはYAMLの各コントローラ名の`type:`。ハードウェアはURDFの`<ros2_control>`の中の`<plugin>` |
| rviz2 | `rviz_common::Display` | YAMLではなくGUIの「Add」で選ぶ。設定は`.rviz`ファイルに保存される |
| image_transport | `image_transport::PublisherPlugin` / `SubscriberPlugin` | 購読側の`image_transport`パラメータに`raw`や`compressed`などの転送方式の名前 |

nav2のplanner serverは、たとえば次のように書きます（nav2公式ドキュメントの例です）。

```yaml
planner_server:
  ros__parameters:
    planner_plugins: ["GridBased"]
    GridBased:
      plugin: "nav2_navfn_planner::NavfnPlanner"
```

この形、つまり**「名前の列」と「名前ごとの名前空間に`plugin:`とそのプラグインの設定」**が、ROS 2でプラグインを設定するときの標準的な形です。この章の後半で作るパイプラインも、同じ形にします。

なお、プラグインの「名前」は、`plugins.xml`の`name`に書いた文字列です。nav2は`nav2_navfn_planner::NavfnPlanner`のようなC++のクラス名、ros2_controlは`joint_state_broadcaster/JointStateBroadcaster`のような`パッケージ名/クラス名`の形を使います。どちらも`plugins.xml`の`name`と一致していれば動きます。この章では後者の形（`drill/ClampFilter`）にします。

### 3. 作り方

題材は、速度指令（`geometry_msgs/msg/Twist`）にかけるフィルタです。`linear.x`と`angular.z`だけを扱います。3つのフィルタを作ります。

| クラス | 名前 | 動き |
| --- | --- | --- |
| `drill::ClampFilter` | `drill/ClampFilter` | 絶対値を上限で切る |
| `drill::RateLimitFilter` | `drill/RateLimitFilter` | 前回の出力からの変化を、加速度×経過時間までに制限する |
| `drill::DeadbandFilter` | `drill/DeadbandFilter` | 絶対値がしきい値未満なら0にする |

#### 基底クラス: 設定は`initialize()`で受け取る

```cpp
class VelocityFilter
{
public:
  virtual ~VelocityFilter() = default;
  virtual void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) = 0;
  virtual geometry_msgs::msg::Twist filter(
    const geometry_msgs::msg::Twist & command, double dt_seconds) = 0;
protected:
  VelocityFilter() = default;
};
```

ここで大事なのは、**コンストラクタに引数を持たせない**ことです。pluginlibは名前しか知らないので、引数なしのコンストラクタでオブジェクトを作ります。しきい値のような設定を、コンストラクタ引数で渡すことはできません。そこで、作ったあとに呼ぶ`initialize()`で設定を受け取ります。`node`はパラメータを宣言・取得するために渡し、`name`は「このプラグインのYAMLの名前空間」です。プラグインは`<name>.max_linear`のように、自分の名前空間の下でパラメータを宣言します。

デストラクタを`virtual`にするのも約束事です。基底クラスのポインタ越しに`delete`されるためです。

#### プラグインのクラスと`PLUGINLIB_EXPORT_CLASS`

プラグインは基底クラスを継承して、`initialize()`と`filter()`を実装します。ソースファイルの末尾で、`PLUGINLIB_EXPORT_CLASS(実装クラス, 基底クラス)`を呼んで登録します。

```cpp
#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(drill::ClampFilter, drill::VelocityFilter)
```

このマクロが、共有ライブラリを読み込んだときに「`ClampFilter`を`VelocityFilter`として作れる」という登録を行います。クラスの全文は「手元で試す」に載せます。

#### plugins.xml

名前とクラス・ライブラリの対応表を、パッケージ直下の`plugins.xml`に書きます。

```xml
<library path="velocity_filters">
  <class name="drill/ClampFilter" type="drill::ClampFilter"
         base_class_type="drill::VelocityFilter">
    <description>速度の絶対値を上限で切る。</description>
  </class>
</library>
```

- `library path`: ライブラリ名。`lib`の接頭辞と`.so`の拡張子は付けない（CMakeの`add_library(velocity_filters ...)`の名前）
- `class name`: **ホストやYAMLで使う名前**
- `type`: 実装のクラス名。`PLUGINLIB_EXPORT_CLASS`の第1引数と同じ
- `base_class_type`: 基底クラス名。`PLUGINLIB_EXPORT_CLASS`の第2引数と同じ

#### CMakeとpackage.xml

CMakeでは、ライブラリを作り、`pluginlib_export_plugin_description_file`で`plugins.xml`を書き出します。依存は`target_link_libraries`で書きます。

```cmake
find_package(pluginlib REQUIRED)

add_library(velocity_filters SHARED src/velocity_filters.cpp)
target_link_libraries(velocity_filters PUBLIC
  rclcpp::rclcpp ${geometry_msgs_TARGETS} pluginlib::pluginlib)

pluginlib_export_plugin_description_file(velocity_filter_demo plugins.xml)
```

- `ament_target_dependencies`は使いません。Lyricalで削除されたためです。Jazzyでも`target_link_libraries`で書けるので、両方で動く書き方はこれです
- `pluginlib_export_plugin_description_file`の第1引数は、**基底クラスがあるパッケージ**です。この章では基底クラスと実装が同じパッケージなので自分の名前になります。基底クラスを別パッケージに置き、実装だけ別のパッケージで書くときは、基底クラスのあるパッケージの名前を渡します
- この呼び出しを忘れると、`plugins.xml`が探せるように登録されず、ホストは名前を見つけられません（「手元で試す」で実際に起こします）

`package.xml`には`<depend>pluginlib</depend>`を足します。ホストと実装を別のパッケージにするなら、実装のパッケージは基底クラスのパッケージにも`<depend>`します。

### 4. 使い方（ClassLoaderで読み込む）

ホストは、基底クラスのヘッダと`pluginlib/class_loader.hpp`だけを使います。実装のヘッダは要りません。

```cpp
#include "pluginlib/class_loader.hpp"

pluginlib::ClassLoader<drill::VelocityFilter> loader("velocity_filter_demo", "drill::VelocityFilter");
auto filter = loader.createUniqueInstance("drill/ClampFilter");
filter->initialize(node, "clamp");
```

- `ClassLoader`のコンストラクタには、**基底クラスがあるパッケージの名前**と、**基底クラスの名前**を渡します
- `createSharedInstance("名前")`は`std::shared_ptr`を、`createUniqueInstance("名前")`は`pluginlib::UniquePtr`（`std::unique_ptr`の仲間）を返します。所有が1か所なら`createUniqueInstance`が素直です
- `getDeclaredClasses()`は、登録されている名前の一覧を返します。名前を間違えたときのメッセージや、一覧を表示する画面に使えます
- 名前が見つからない、ライブラリが開けない、といった失敗は、**`pluginlib::PluginlibException`**で知らせてきます。`what()`のメッセージに、探した名前と登録されている名前が入っています。**これを受けて、分かるメッセージで落とすか、飛ばすかを決める**のがホストの仕事です。`isClassAvailable("名前")`で先に調べる方法もあります

#### ClassLoaderをインスタンスより長生きさせる

`createUniqueInstance`や`createSharedInstance`で作ったオブジェクトのコードは、共有ライブラリの中にあります。`ClassLoader`が先に壊れると、ライブラリを外そうとするので、まだ生きているオブジェクトのコードが消えてしまいます。そのため、**`ClassLoader`は、そこから作ったインスタンスより後に壊れなければなりません**。

C++のクラスのメンバは、**宣言の逆順に壊れます**。だから、`ClassLoader`のメンバを、インスタンスを持つメンバより**先に**宣言します。

```cpp
private:
  pluginlib::ClassLoader<VelocityFilter> loader_;                   // 先に宣言 = 後に壊れる
  std::vector<pluginlib::UniquePtr<VelocityFilter>> filters_;       // 後に宣言 = 先に壊れる
```

順番を逆にするとどうなるかは、「手元で試す」で実測します。

### 5. YAMLの設定方法

設定の形は、nav2と同じにします。**使うプラグインの名前の列**（`filters`）と、**名前ごとの名前空間**にプラグインのクラス名（`plugin`）と設定です。

```yaml
filter_host:
  ros__parameters:
    filters: ["deadband", "clamp", "rate_limit"]   # この順に通す
    deadband:
      plugin: "drill/DeadbandFilter"
      linear_threshold: 0.05
    clamp:
      plugin: "drill/ClampFilter"
      max_linear: 0.5
    rate_limit:
      plugin: "drill/RateLimitFilter"
      max_linear_acceleration: 1.0
```

ホストは、`filters`と各`<名前>.plugin`を宣言して読み込み、`initialize(node, 名前)`を呼びます。各プラグインは、自分の`<名前>.<項目>`を宣言します。たとえば`clamp`のプラグインは、`clamp.max_linear`と`clamp.max_angular`を宣言します。

#### よい使い方

- **プラグインごとに名前空間を分ける。** `deadband.linear_threshold`のように、名前の下に置きます。トップに`max_linear: 0.5`のように置くと、別のプラグインの同名の項目とぶつかります。名前は**クラス名ではなくインスタンスの名前**なので、同じクラスを別の設定で2回使うこともできます（`clamp_slow`と`clamp_fast`など）。ただし、名前は重複させません
- **既定値を持たせる。** プラグインは`declare_parameter`で既定値を持ち、YAMLには変えたい項目だけを書きます。上の例の`deadband`は`angular_threshold`を書いていませんが、既定値の`0.1`で動きます。YAMLが短くなり、書き忘れで起動できない、ということも減ります
- **未知の名前をどう扱うか、決めておく。** `plugin`の名前が登録されていないとき、「分かるメッセージで落とす」か「そのプラグインを飛ばして続ける」かを、ホストの設計として決めます。速度の安全装置のように、効いていないと困るものは**落とす**のが安全です（飛ばすと、フィルタが効かないまま動いてしまいます）。飾りの部品なら飛ばしてもかまいません。どちらにしても、黙って何も起こらないのは避け、メッセージを出します
- **順番に意味がある列は、YAMLの配列で書く。** この章の`filters`のように、通す順番が結果を変える場合は、配列の順番を使います。名前ごとのブロックの並び順に頼ってはいけません（パラメータは名前の辞書なので、書いた順は保証されません）

#### 避けたい使い方

- 差し替えが要らないのにプラグインにする（実装が1つしかない、など）
- クラス名をC++のコードに直接書く。それでは、設定で差し替えられません
- 1つの文字列に設定を詰め込む（`"clamp:0.5"`のような形）。型のあるパラメータとして書けば、型の食い違いも検査されます
- 全項目を必須にして既定値を持たせない。YAMLが長くなり、1つ書き忘れただけで起動しなくなります
- 未知の名前を黙って飛ばす。タイプミスに気づけません
- 数値の型をそろえない。`max_linear: 1`のように小数点を省くと整数になり、`double`で宣言したパラメータには入りません（「手元で試す」で実測します）

## 手元で試す

`~/ros2_ws/src/`に、基底クラスと3つのフィルタ、YAMLでパイプラインを組むホストをまとめた小さなパッケージ`velocity_filter_demo`を作ります。ファイルは全部で7つです。最初に空のパッケージを作り、あとは次のファイルの内容を置いていきます。

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake --license Apache-2.0 velocity_filter_demo
mkdir -p velocity_filter_demo/include/drill velocity_filter_demo/config
```

`ros2 pkg create`が作る`package.xml`と`CMakeLists.txt`は、次の内容で置き換えます。

```xml
<?xml version="1.0"?>
<!-- ~/ros2_ws/src/velocity_filter_demo/package.xml -->
<package format="3">
  <name>velocity_filter_demo</name>
  <version>0.1.0</version>
  <description>pluginlib demo: velocity filters</description>
  <maintainer email="you@example.com">you</maintainer>
  <license>Apache-2.0</license>

  <buildtool_depend>ament_cmake</buildtool_depend>

  <depend>rclcpp</depend>
  <depend>geometry_msgs</depend>
  <depend>pluginlib</depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
```

```cmake
# ~/ros2_ws/src/velocity_filter_demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.8)
project(velocity_filter_demo)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(pluginlib REQUIRED)

# プラグインの実装。ホストはこのライブラリにリンクしない（実行時に読み込む）。
add_library(velocity_filters SHARED src/velocity_filters.cpp)
target_include_directories(velocity_filters PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(velocity_filters PUBLIC
  rclcpp::rclcpp ${geometry_msgs_TARGETS} pluginlib::pluginlib)

# ホスト。基底クラスのヘッダと pluginlib だけを使う。
add_executable(filter_host src/filter_host.cpp)
target_include_directories(filter_host PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(filter_host
  rclcpp::rclcpp ${geometry_msgs_TARGETS} pluginlib::pluginlib)

install(TARGETS velocity_filters LIBRARY DESTINATION lib)
install(TARGETS filter_host DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})

# plugins.xml をほかのパッケージから見つけられるように書き出す。
pluginlib_export_plugin_description_file(velocity_filter_demo plugins.xml)

ament_package()
```

基底クラスです。ホストと、すべてのプラグインがこのヘッダだけを共有します。

```cpp
// ~/ros2_ws/src/velocity_filter_demo/include/drill/velocity_filter.hpp
#ifndef DRILL_VELOCITY_FILTER_HPP_
#define DRILL_VELOCITY_FILTER_HPP_

#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"

namespace drill
{

class VelocityFilter
{
public:
  virtual ~VelocityFilter() = default;

  // pluginlib は引数なしのコンストラクタで作るので、設定はここで受け取る。
  // name は YAML の名前空間（例: "clamp"）。パラメータは "<name>.<項目>" で宣言する。
  virtual void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) = 0;

  // dt_seconds は前回の指令からの経過時間。
  virtual geometry_msgs::msg::Twist filter(
    const geometry_msgs::msg::Twist & command, double dt_seconds) = 0;

protected:
  VelocityFilter() = default;
};

}  // namespace drill

#endif  // DRILL_VELOCITY_FILTER_HPP_
```

3つのフィルタと、そのエクスポートです。

```cpp
// ~/ros2_ws/src/velocity_filter_demo/src/velocity_filters.cpp
#include <algorithm>
#include <cmath>
#include <string>

#include "drill/velocity_filter.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace drill
{

// 絶対値を上限で切る。
class ClampFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    max_linear_ = node->declare_parameter<double>(name + ".max_linear", 1.0);
    max_angular_ = node->declare_parameter<double>(name + ".max_angular", 2.0);
  }

  geometry_msgs::msg::Twist filter(const geometry_msgs::msg::Twist & command, double) override
  {
    geometry_msgs::msg::Twist result = command;
    result.linear.x = std::clamp(command.linear.x, -max_linear_, max_linear_);
    result.angular.z = std::clamp(command.angular.z, -max_angular_, max_angular_);
    return result;
  }

private:
  double max_linear_ = 1.0;
  double max_angular_ = 2.0;
};

// 前回の出力からの変化を、加速度 × dt までに制限する。
class RateLimitFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    max_linear_acceleration_ = node->declare_parameter<double>(name + ".max_linear_acceleration", 0.5);
    max_angular_acceleration_ = node->declare_parameter<double>(name + ".max_angular_acceleration", 1.0);
  }

  geometry_msgs::msg::Twist filter(const geometry_msgs::msg::Twist & command, double dt_seconds) override
  {
    geometry_msgs::msg::Twist result = command;
    result.linear.x = approach(previous_.linear.x, command.linear.x, max_linear_acceleration_ * dt_seconds);
    result.angular.z = approach(previous_.angular.z, command.angular.z, max_angular_acceleration_ * dt_seconds);
    previous_ = result;
    return result;
  }

private:
  static double approach(double previous, double target, double max_step)
  {
    return previous + std::clamp(target - previous, -max_step, max_step);
  }

  double max_linear_acceleration_ = 0.5;
  double max_angular_acceleration_ = 1.0;
  geometry_msgs::msg::Twist previous_;  // 最初の 1 回は 0 からの変化として扱う
};

// 絶対値がしきい値未満なら 0 にする。
class DeadbandFilter : public VelocityFilter
{
public:
  void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) override
  {
    linear_threshold_ = node->declare_parameter<double>(name + ".linear_threshold", 0.05);
    angular_threshold_ = node->declare_parameter<double>(name + ".angular_threshold", 0.1);
  }

  geometry_msgs::msg::Twist filter(const geometry_msgs::msg::Twist & command, double) override
  {
    geometry_msgs::msg::Twist result = command;
    if (std::abs(command.linear.x) < linear_threshold_) {
      result.linear.x = 0.0;
    }
    if (std::abs(command.angular.z) < angular_threshold_) {
      result.angular.z = 0.0;
    }
    return result;
  }

private:
  double linear_threshold_ = 0.05;
  double angular_threshold_ = 0.1;
};

}  // namespace drill

// 第 1 引数が実装、第 2 引数が基底クラス。plugins.xml の type / base_class_type と同じ名前にする。
PLUGINLIB_EXPORT_CLASS(drill::ClampFilter, drill::VelocityFilter)
PLUGINLIB_EXPORT_CLASS(drill::RateLimitFilter, drill::VelocityFilter)
PLUGINLIB_EXPORT_CLASS(drill::DeadbandFilter, drill::VelocityFilter)
```

名前の対応表です。

```xml
<?xml version="1.0"?>
<!-- ~/ros2_ws/src/velocity_filter_demo/plugins.xml -->
<library path="velocity_filters">
  <class name="drill/ClampFilter" type="drill::ClampFilter"
         base_class_type="drill::VelocityFilter">
    <description>速度の絶対値を上限で切る。</description>
  </class>
  <class name="drill/RateLimitFilter" type="drill::RateLimitFilter"
         base_class_type="drill::VelocityFilter">
    <description>前回の出力からの変化を制限する。</description>
  </class>
  <class name="drill/DeadbandFilter" type="drill::DeadbandFilter"
         base_class_type="drill::VelocityFilter">
    <description>小さい速度を 0 にする。</description>
  </class>
</library>
```

ホストです。YAMLの`filters`の順にプラグインを読み込み、3つの指令を順に通して結果を表示します。

```cpp
// ~/ros2_ws/src/velocity_filter_demo/src/filter_host.cpp
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "drill/velocity_filter.hpp"
#include "pluginlib/class_loader.hpp"
#include "rclcpp/rclcpp.hpp"

namespace drill
{

// YAML の "filters" の順にプラグインを読み込み、順に通すパイプライン。
class FilterPipeline
{
public:
  explicit FilterPipeline(const rclcpp::Node::SharedPtr & node)
  : loader_("velocity_filter_demo", "drill::VelocityFilter")
  {
    // 既定値は空の列（何も通さない = そのまま通す）。
    const auto filter_names =
      node->declare_parameter<std::vector<std::string>>("filters", std::vector<std::string>{});
    for (const auto & filter_name : filter_names) {
      filters_.push_back(loadFilter(node, filter_name));
    }
  }

  geometry_msgs::msg::Twist apply(const geometry_msgs::msg::Twist & command, double dt_seconds)
  {
    geometry_msgs::msg::Twist result = command;
    for (const auto & filter : filters_) {
      result = filter->filter(result, dt_seconds);
    }
    return result;
  }

  std::vector<std::string> declaredClasses() {return loader_.getDeclaredClasses();}

private:
  pluginlib::UniquePtr<VelocityFilter> loadFilter(
    const rclcpp::Node::SharedPtr & node, const std::string & filter_name)
  {
    const auto plugin_name = node->declare_parameter<std::string>(filter_name + ".plugin", "");
    if (plugin_name.empty()) {
      throw std::runtime_error(
              "フィルタ '" + filter_name + "': パラメータ '" + filter_name + ".plugin' が設定されていません");
    }
    try {
      auto filter = loader_.createUniqueInstance(plugin_name);
      filter->initialize(node, filter_name);
      return filter;
    } catch (const pluginlib::PluginlibException & exception) {
      throw std::runtime_error(
              "フィルタ '" + filter_name + "': '" + plugin_name + "' を読み込めません: " + exception.what());
    }
  }

  // メンバは宣言の逆順に壊れる。loader_ を filters_ より先に宣言して、
  // ClassLoader がインスタンスより長生きするようにする。
  pluginlib::ClassLoader<VelocityFilter> loader_;
  std::vector<pluginlib::UniquePtr<VelocityFilter>> filters_;
};

}  // namespace drill

int main(int argc, char ** argv)
{
  constexpr double kStepSeconds = 0.1;

  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("filter_host");
  int exit_code = 0;
  try {
    drill::FilterPipeline pipeline(node);

    std::cout << "登録されている名前:";
    for (const auto & class_name : pipeline.declaredClasses()) {
      std::cout << " " << class_name;
    }
    std::cout << std::endl;

    const std::vector<std::pair<double, double>> commands = {{0.8, 0.05}, {0.8, 0.5}, {0.02, 0.5}};
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t index = 0; index < commands.size(); ++index) {
      geometry_msgs::msg::Twist command;
      command.linear.x = commands[index].first;
      command.angular.z = commands[index].second;
      const auto result = pipeline.apply(command, kStepSeconds);
      std::cout << "ステップ " << index + 1 << ": 入力 (" << command.linear.x << ", " << command.angular.z
                << ") -> 出力 (" << result.linear.x << ", " << result.angular.z << ")" << std::endl;
    }
  } catch (const std::exception & exception) {
    RCLCPP_ERROR(node->get_logger(), "%s", exception.what());
    exit_code = 1;
  }
  rclcpp::shutdown();
  return exit_code;
}
```

最後に、設計の節で見たYAMLです。

```yaml
# ~/ros2_ws/src/velocity_filter_demo/config/filters.yaml
filter_host:
  ros__parameters:
    filters: ["deadband", "clamp", "rate_limit"]   # この順に通す
    deadband:
      plugin: "drill/DeadbandFilter"
      linear_threshold: 0.05
    clamp:
      plugin: "drill/ClampFilter"
      max_linear: 0.5
    rate_limit:
      plugin: "drill/RateLimitFilter"
      max_linear_acceleration: 1.0
```

### 動かす

ビルドして、YAMLを渡して実行します。

```bash
cd ~/ros2_ws
colcon build --packages-select velocity_filter_demo
source install/setup.bash
ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml
```

**予想: 3つの指令`(0.8, 0.05)`、`(0.8, 0.5)`、`(0.02, 0.5)`（`linear.x`, `angular.z`）が、`deadband`、`clamp`、`rate_limit`の順に通ると、それぞれ何になるでしょうか。（時間は1ステップ0.1秒です。`rate_limit`は、`max_linear_acceleration`が1.0で、`max_angular_acceleration`は書いていないので既定値の1.0です。）**

<details markdown="1"><summary>解答（実行結果）</summary>

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml
登録されている名前: drill/ClampFilter drill/DeadbandFilter drill/RateLimitFilter
ステップ 1: 入力 (0.80, 0.05) -> 出力 (0.10, 0.00)
ステップ 2: 入力 (0.80, 0.50) -> 出力 (0.20, 0.10)
ステップ 3: 入力 (0.02, 0.50) -> 出力 (0.10, 0.20)
```

最初の行は、`getDeclaredClasses()`が返した、登録されている名前の一覧です。

- ステップ1: `angular.z`の`0.05`は、`deadband`のしきい値`0.1`より小さいので`0`になります。`linear.x`の`0.8`は、`clamp`で`0.5`に切られたあと、`rate_limit`が0からの変化を`1.0 × 0.1 = 0.1`までに制限するので`0.10`になります
- ステップ2: `linear.x`は前回の`0.10`から`0.1`だけ増えて`0.20`に、`angular.z`は`0`から`0.1`だけ増えて`0.10`になります
- ステップ3: `linear.x`の`0.02`は`deadband`で`0`になり、`rate_limit`が`0.20`から`0.1`だけ戻して`0.10`にします。`angular.z`は`0.10`から`0.20`に増えます

実装のヘッダを一度もインクルードしていない`filter_host`が、3つのフィルタを動かしました。実装は`velocity_filters`ライブラリの中にあり、`filter_host`は実行時に名前だけで読み込んでいます。

</details>

### 順番を変える

YAMLの`filters`の順番を変えると、同じ指令でも結果が変わります。コマンドラインから`-p`で`filters`だけ上書きして試します。

```bash
ros2 run velocity_filter_demo filter_host --ros-args \
  --params-file src/velocity_filter_demo/config/filters.yaml \
  -p 'filters:=[rate_limit, clamp, deadband]'
```

**予想: `rate_limit`が先頭に来ると、ステップ2の`angular.z`はどうなるでしょうか。**

<details markdown="1"><summary>解答（実行結果）</summary>

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[rate_limit, clamp, deadband]'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p 'filters:=[rate_limit, clamp, deadband]'" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[rate_limit, clamp, deadband]
登録されている名前: drill/ClampFilter drill/DeadbandFilter drill/RateLimitFilter
ステップ 1: 入力 (0.80, 0.05) -> 出力 (0.10, 0.00)
ステップ 2: 入力 (0.80, 0.50) -> 出力 (0.20, 0.15)
ステップ 3: 入力 (0.02, 0.50) -> 出力 (0.10, 0.25)
```

先頭の`rate_limit`は、`deadband`で消される前の`angular.z`（ステップ1では`0.05`）を前回の出力として覚えます。そのため、ステップ2では`0.05`から`0.1`だけ増えた`0.15`になり、`deadband`のしきい値`0.1`を超えて残ります。順番に意味がある、というのはこういうことです。だから、通す順番は`filters`の配列で明示します。

</details>

### 失敗を見る

プラグインの失敗は、メッセージの読み方を知っていると直せます。実際に起こして見ます。

#### 未知の名前を読み込む

`clamp`の`plugin`を、登録されていない名前に上書きします。

```bash
ros2 run velocity_filter_demo filter_host --ros-args \
  --params-file src/velocity_filter_demo/config/filters.yaml \
  -p clamp.plugin:=drill/NoSuchFilter
```

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p clamp.plugin:=drill/NoSuchFilter'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p clamp.plugin:=drill/NoSuchFilter" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p clamp.plugin:=drill/NoSuchFilter
[ERROR] [1791032103.623048588] [filter_host]: フィルタ 'clamp': 'drill/NoSuchFilter' を読み込めません: According to the loaded plugin descriptions the class drill/NoSuchFilter with base class type drill::VelocityFilter does not exist. Declared types are  drill/ClampFilter drill/DeadbandFilter drill/RateLimitFilter
[ros2run]: Process exited with failure 1
```

`PluginlibException`の`what()`は、「その名前のクラスは、基底クラス`drill::VelocityFilter`として存在しない」と、そのとき**登録されている名前の一覧**（`Declared types are`以降）を教えてくれます。ホストはこれに、どのフィルタ（`clamp`）の名前だったかを足して、終了コード1で落ちます。名前のタイプミスなら、一覧と見比べればすぐ分かります。

`plugin`を書き忘れた場合は、ホストが自分で検査して、別のメッセージを出します。

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[deadband, mystery]'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p 'filters:=[deadband, mystery]'" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[deadband, mystery]
[ERROR] [1791032089.404233179] [filter_host]: フィルタ 'mystery': パラメータ 'mystery.plugin' が設定されていません
[ros2run]: Process exited with failure 1
```

`filters`に`mystery`を足したのに、`mystery.plugin`を書いていない場合です。

#### plugins.xmlを書き出し忘れる

CMakeLists.txtから`pluginlib_export_plugin_description_file`の行を消してビルドし直します。ビルドは通ります。ところが、実行すると、プラグインが1つも見つかりません。

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="sed -i '/pluginlib_export_plugin_description_file/d' src/velocity_filter_demo/CMakeLists.txt; colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml
[ERROR] [1791032074.156689047] [filter_host]: フィルタ 'deadband': 'drill/DeadbandFilter' を読み込めません: According to the loaded plugin descriptions the class drill/DeadbandFilter with base class type drill::VelocityFilter does not exist. Declared types are
[ros2run]: Process exited with failure 1
```

メッセージは前の「未知の名前」と同じ形ですが、`Declared types are`のあとが**空**です。名前が違うのではなく、`plugins.xml`が登録されていないことを示しています。プラグインが1つも見えないときは、まずこれを疑います。`pluginlib_export_plugin_description_file`を呼ぶと、`install`の下に`<パッケージ名>__pluginlib__plugin`という名前の索引が作られます。ビルドの結果は、次のコマンドで確かめられます。

```bash
ls install/velocity_filter_demo/share/ament_index/resource_index | grep pluginlib
```

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; echo '$ ls install/velocity_filter_demo/share/ament_index/resource_index | grep pluginlib'; ls install/velocity_filter_demo/share/ament_index/resource_index | grep pluginlib" -->
```
$ ls install/velocity_filter_demo/share/ament_index/resource_index | grep pluginlib
velocity_filter_demo__pluginlib__plugin
```

この行が出ないなら、`pluginlib_export_plugin_description_file`が呼ばれていません。

#### ClassLoaderを先に壊す

`filter_host.cpp`の2つのメンバの宣言順を入れ替えます（`filters_`を先、`loader_`を後にします）。

```cpp
  std::vector<pluginlib::UniquePtr<VelocityFilter>> filters_;       // 先に宣言 = 後に壊れる
  pluginlib::ClassLoader<VelocityFilter> loader_;                   // 後に宣言 = 先に壊れる（よくない）
```

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="sed -i -e '/^  pluginlib::ClassLoader<VelocityFilter> loader_;/{h;d}' -e '/^  std::vector<pluginlib::UniquePtr<VelocityFilter>> filters_;/G' src/velocity_filter_demo/src/filter_host.cpp; colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml
登録されている名前: drill/ClampFilter drill/DeadbandFilter drill/RateLimitFilter
ステップ 1: 入力 (0.80, 0.05) -> 出力 (0.10, 0.00)
ステップ 2: 入力 (0.80, 0.50) -> 出力 (0.20, 0.10)
ステップ 3: 入力 (0.02, 0.50) -> 出力 (0.10, 0.20)
Warning: class_loader.ClassLoader: SEVERE WARNING!!! Attempting to unload library while objects created by this loader exist in the heap! You should delete your objects before attempting to unload the library or destroying the ClassLoader. The library will NOT be unloaded.
         at line 127 in ./src/class_loader.cpp
```

この実測では、結果の表示は正しいまま、終了時に`class_loader`が警告を出しました。「ライブラリはまだオブジェクトが生きているので外さない」という警告です。`class_loader`が、落ちないようにライブラリを外さないでいてくれた形ですが、約束（`ClassLoader`はインスタンスより長生き）を破っている印です。警告が出ない並びにしておきます。

#### YAMLの型を間違える

`max_linear: 1`のように、小数点を省くと整数になります。`double`で宣言したパラメータには入りません。

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p clamp.max_linear:=1'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p clamp.max_linear:=1" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p clamp.max_linear:=1
[ERROR] [1791032034.396037125] [filter_host]: parameter 'clamp.max_linear' has invalid type: Wrong parameter type, parameter {clamp.max_linear} is of type {double}, setting it to {integer} is not allowed.
[ros2run]: Process exited with failure 1
```

`1.0`と書けば通ります。

#### 同じ名前を2回使う

`filters`に同じ名前を2回書くと、2回目の`declare_parameter`で落ちます。同じクラスを2回使いたいときは、別の名前（と別の名前空間）にします。

<!-- measure: env=ros files=src/velocity_filter_demo/package.xml,src/velocity_filter_demo/CMakeLists.txt,src/velocity_filter_demo/include/drill/velocity_filter.hpp,src/velocity_filter_demo/src/velocity_filters.cpp,src/velocity_filter_demo/plugins.xml,src/velocity_filter_demo/src/filter_host.cpp,src/velocity_filter_demo/config/filters.yaml cmd="colcon build --packages-select velocity_filter_demo >/dev/null 2>&1; source install/setup.bash; echo '$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[clamp, clamp]'; ros2 run velocity_filter_demo filter_host --ros-args --params-file src/velocity_filter_demo/config/filters.yaml -p 'filters:=[clamp, clamp]'" -->
```
$ ros2 run velocity_filter_demo filter_host --ros-args --params-file ... -p filters:=[clamp, clamp]
[ERROR] [1791032020.349085717] [filter_host]: parameter 'clamp.plugin' has already been declared
[ros2run]: Process exited with failure 1
```

## つまずきポイント

- **`plugins.xml`は書いただけでは効きません。** CMakeLists.txtの`pluginlib_export_plugin_description_file`で書き出し、`package.xml`に`pluginlib`の依存を書きます。忘れると、「登録されている名前が空」のメッセージになります
- **名前を3か所で合わせます。** `plugins.xml`の`type`と`base_class_type`は、`PLUGINLIB_EXPORT_CLASS`の2つの引数と、名前空間も含めて一致させます。ホストの`ClassLoader`の第2引数も、`base_class_type`と同じにします
- **`library path`は、ライブラリの名前から`lib`と`.so`を除いたものです。** `add_library`の名前と同じです
- **`ClassLoader`の第1引数は、基底クラスのあるパッケージです。** プラグインを書いたパッケージではありません。`pluginlib_export_plugin_description_file`の第1引数も同じ考え方です
- **コンストラクタで設定を受け取れません。** 引数なしで作られるので、`initialize()`で受け取ります
- **`ClassLoader`は、インスタンスを持つメンバより先に宣言します。** 逆にすると、実測のように終了時に警告が出ます。ローカル変数で使うときも、`ClassLoader`のほうを先に作ります
- **`ament_target_dependencies`はLyricalで削除されました。** `target_link_libraries`で書きます
- **YAMLの数値は型まで合わせます。** `double`のパラメータには`1.0`と書きます
- **プラグインごとの名前空間を分けます。** 名前が重なると、宣言済みの例外で落ちます

## おわりに

pluginlibは、「基底クラスと名前の対応表を決めておき、実装は設定から選ぶ」ための仕組みです。この章のパイプラインは、nav2のplannerやcontrollerと同じ形（名前の列と、名前ごとの名前空間）で組んでいます。次に標準パッケージのYAMLを見るときは、`plugin:`の行がどのクラスを選んでいるのか、そのクラスの基底クラスは何か、を追ってみてください。

次は[16_アクションサーバの実装](16_アクションサーバの実装.md)で、feedback付きの長時間タスクをサーバ側から実装します。わからないところがあれば、周りの経験者に聞くか、公式ドキュメントで確かめましょう。

### 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `16_pluginlib_create` — プラグインを作って書き出す
- `17_pluginlib_load` — ClassLoaderで読み込む
- `18_pluginlib_yaml` — YAMLのパラメータでプラグインの列を組む

```bash
./drill run 16
./drill run 17
./drill run 18
```

課題側からは `./drill read` でこの章に戻ってこられます。

## 資料

- [Creating and using plugins (C++) — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
- [pluginlib — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/p/pluginlib/)
- [Writing a new hardware component — ros2_control Documentation: Jazzy](https://control.ros.org/jazzy/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html)
- [Planner Server — Nav2 Documentation](https://docs.nav2.org/rolling/configuration_and_development/configuration_guide/core_servers/configuring_planner_server/)
- [04_ノード](04_ノード.md)
- 前回: [15_パラメータとlaunchの実践](15_パラメータとlaunchの実践.md)
- 次回: [16_アクションサーバの実装](16_アクションサーバの実装.md)
