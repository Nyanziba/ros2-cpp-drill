# 課題 15: コンポーネント化する（RCLCPP_COMPONENTS）〔上級〕

公式チュートリアル
[Writing a Composable Node (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Writing-a-Composable-Node.html)
と
[Composing multiple nodes in a single process](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Composition.html)
の内容を、`ComposableTalker` という 1 つのクラスに凝縮しました。

これまでの課題ではノードを 1 つずつ `add_executable` で実行ファイルにし、
`ros2 run` でプロセスとして起動してきました。この課題ではそのやり方を
やめ、**「1 プロセス 1 ノード」を強制しない**書き方を学びます。

## やること

`src/composable_talker.cpp` の TODO を埋めてください。仕様は課題01の
`MinimalPublisher` とほぼ同じですが、守るべき約束が 1 つ増えます。

| 項目 | 値 |
| --- | --- |
| クラス名 | `ComposableTalker`（namespace なし） |
| 既定のノード名 | `composable_talker` |
| トピック名 | `topic` |
| 型 | `std_msgs::msg::String` |
| QoS depth | 10 |
| 周期 | 200ms |
| 本文 | `"composable hello " + std::to_string(count_++)` |
| ログ | `Publishing: '<本文>'` |

クラス宣言（`include/drill/composable_talker.hpp`）は与えてあります。
`publisher_` / `timer_` / `count_` に何を入れるかは課題01と同じ考え方です。

TODO は次の 3 箇所です。

1. **コンストラクタが `options` を `Node` にそのまま渡すこと。**
   `explicit ComposableTalker(const rclcpp::NodeOptions & options)` は
   受け取った `options` を捨てずに `Node("composable_talker", options)` と
   渡さなければなりません。
2. **Publisher とタイマを作ること。** 課題01と同じです。
3. **ソースの末尾で登録すること。**
   ```cpp
   #include "rclcpp_components/register_node_macro.hpp"
   RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker)
   ```
   この行を書かせるのがこの課題の核心です。他の 2 つを完璧に埋めても、
   この登録がなければ `component_container` はこのクラスの存在に
   気づけません。

## NodeOptions を渡す意味

`component_container` はこのクラスを `new ComposableTalker()` と直接
インスタンス化するのではなく、`RCLCPP_COMPONENTS_REGISTER_NODE` が生成した
ファクトリ経由で `NodeOptions` を組み立ててから生成します。この
`NodeOptions` には、実行時に決まる次のような設定が積まれています。

- `--ros-args -r __node:=foo` のようなノード名・トピック名のリマップ
- `--ros-args -p param:=value` で渡すパラメータ
- `use_intra_process_comms(true)`（課題14 のゼロコピー通信）。コンポーネント
  は同一プロセスに複数ノードを載せられる仕組みなので、ゼロコピーが効くか
  どうかは NodeOptions 経由でしか制御できません。

コンストラクタが `options` を受け取っても `Node("composable_talker")` と
決め打ちしてしまうと、これらの設定は一切反映されません。

## コンポーネントの利点

- **プロセス内通信でゼロコピーが効く。** 課題14 で見た
  `use_intra_process_comms(true)` は、Publisher と Subscription が
  同一プロセスにいて初めて効きます。ノードを実行ファイルに分けていると
  DDS 越しの通信になり、恩恵を受けられません。コンポーネントなら
  複数ノードを 1 プロセスにまとめられるので、この最適化が可能になります。
- **起動構成を launch で組み替えられる。** どのノードをどのプロセスに
  まとめるかは、C++ のコードを直さなくても launch ファイル側の記述だけで
  変更できます（下の launch の例を参照）。
- **プロセス数を減らせる。** ノードの数だけプロセスを立てるとコンテキスト
  スイッチやメモリのオーバーヘッドが積み上がります。関連の深いノードを
  1 プロセスにまとめれば、それらを減らせます。

## `add_executable` の実行ファイルとの違い

これまでの課題（例えば課題01の `talker`）は `add_executable` で作る
「普通の実行ファイル」でした。`main()` の中で `rclcpp::spin()` を呼ぶだけの、
そのノード専用のプロセスです。他のノードと同じプロセスに同居させることは
できません。

コンポーネントは違います。`ComposableTalker` は `add_library(... SHARED)`
で作る**共有ライブラリ**の一部としてビルドされ、`main()` を持ちません。
`component_container`（それ自体はただの実行ファイル）が起動時または
実行時にこの `.so` を `dlopen` し、`RCLCPP_COMPONENTS_REGISTER_NODE` が
登録したファクトリ経由でインスタンスを生成して、自分の Executor に
乗せて spin します。同じコンテナに何個でも別のコンポーネントを
追加でロードでき、それらは全部同じプロセス・同じ Executor で動きます。

## CMake 側の仕組み

`CMakeLists.txt` では次の 2 行が肝です。

```cmake
add_library(${PROJECT_NAME}_node SHARED src/composable_talker.cpp)
...
rclcpp_components_register_node(${PROJECT_NAME}_node
  PLUGIN "ComposableTalker"
  EXECUTABLE composable_talker_node
)
```

- `add_library(... SHARED)`: 実行ファイルではなく共有ライブラリとして
  ビルドします。`dlopen` できる形にするために必須です。
- `rclcpp_components_register_node(...)`: 2 つのことをしてくれます。
  1. ament のリソースインデックス
     （`share/ament_index/resource_index/rclcpp_components/<パッケージ名>`）
     に、`PLUGIN` で指定したプラグイン名とライブラリの場所を書き込みます。
     `component_container` や `ros2 component types` はこのインデックスを
     見てロード可能なコンポーネントを探します。
  2. `EXECUTABLE` で指定した名前の実行ファイル（今回は
     `composable_talker_node`）を自動生成します。中身は
     `class_loader` でこの `.so` を読み込んで単一ノードとして spin する
     だけの薄いラッパーで、`add_executable` を自分で書く必要はありません。

実際に `component_container` がコンポーネントを見つけられるかどうかは、
最終的にソースコード側の `RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker)`
（`class_loader` のプラグイン登録マクロ）が書かれているかにかかっています。
CMake 側の設定だけでは、登録マクロを書き忘れたクラスを見つけることは
できません。

## 手動確認の手順

テストが通ったら、実際に `component_container` へ動的にロードして
確かめてみましょう。まず別の端末でコンテナを起動します。

```bash
ros2 run rclcpp_components component_container         # 別端末
```

もう 1 つの端末から、ロード可能なコンポーネント一覧の確認とロードを
行います。

```bash
ros2 component list
ros2 component load /ComponentManager drill_15_composition ComposableTalker
ros2 topic echo /topic
```

`ros2 component list` を打った時点ではまだ何もロードされていないので
空です。`load` した後にもう一度打つと、コンテナのプロセスの下に
`ComposableTalker` がぶら下がっているのが見えます。

単体で動かすだけなら、`rclcpp_components_register_node` が自動生成した
実行ファイルも使えます。

```bash
ros2 run drill_15_composition composable_talker_node
```

## launch から複数コンポーネントを 1 プロセスに載せる

`ComposableNodeContainer` と `ComposableNode` を使うと、複数のコンポーネント
を 1 つのプロセスにまとめて起動する launch ファイルを書けます。

```python
from launch import LaunchDescription
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode


def generate_launch_description():
    container = ComposableNodeContainer(
        name='drill_container',
        namespace='',
        package='rclcpp_components',
        executable='component_container',
        composable_node_descriptions=[
            ComposableNode(
                package='drill_15_composition',
                plugin='ComposableTalker',
                name='composable_talker',
                # プロセス内通信（課題14 のゼロコピー）は既定で無効。
                # 使いたければここで明示的に有効にする。
                extra_arguments=[{'use_intra_process_comms': True}],
            ),
            # 他のコンポーネントも同じ container に追加すれば、
            # 同一プロセス・同一 Executor で動く。
        ],
        output='screen',
    )
    return LaunchDescription([container])
```

**注意: コンポーネント化しただけではゼロコピーになりません。** コンポーネントは
「同一プロセスに載せる」という前提条件を満たすだけで、プロセス内通信そのものは
`use_intra_process_comms` を明示的に有効にして初めて効きます
（`rclcpp/node_options.hpp` の既定値は `false`）。公式の
`composition_demo_launch.py` も有効化していないので、あの launch で起動した
talker / listener は同一プロセスにいても DDS 経由で通信しています。

CLI から読み込む場合も同じで、公式チュートリアルは明示的に渡しています。

```bash
ros2 component load /ComponentManager composition composition::Talker \
  -e use_intra_process_comms:=true
```

`add_executable` で作ったノードを別プロセスとして同じ launch から
起動することもできますが、その場合は上のようにプロセス内に同居させる
ことはできません。「同じプロセスに載せるかどうか」を launch ファイル
だけで選べるのが、コンポーネントの最大の利点です。

## つまずきポイント

- `Node("composable_talker")` と `options` を渡し忘れると、`--ros-args` での
  リマップやパラメータ指定が効かないノードになります（テスト1で検出）。
- `RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker)` を書き忘れると、
  ビルド自体は普通に通ってしまいます。共有ライブラリとして完成しては
  いるものの、`component_container` からはロードできない「見えない」
  コンポーネントになります（テスト3で検出）。
- 登録マクロの引数（`ComposableTalker`）は、`CMakeLists.txt` の
  `rclcpp_components_register_node(... PLUGIN "ComposableTalker" ...)` の
  文字列と一致していなければなりません。

## テスト

```bash
./drill run 15
```

| テスト | 見ているところ |
| --- | --- |
| `NodeOptionsがNodeに渡されている` | `options` を `Node(...)` に渡しているか（渡さないとノード名リマップが効かない） |
| `topicトピックにpublishしている` | Publisher とタイマが動いているか、本文の組み立て |
| `RCLCPP_COMPONENTS_REGISTER_NODEで登録されている` | 登録マクロが実際に書かれているか（`class_loader` で共有ライブラリを直接読んで確認） |

## 参考

- 公式: [Writing a Composable Node (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Writing-a-Composable-Node.html)
- 公式: [Composing multiple nodes in a single process](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Composition.html)
- ローカルの実装例: `/opt/ros/jazzy/share/composition/`, `/opt/ros/jazzy/lib/composition/`
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
