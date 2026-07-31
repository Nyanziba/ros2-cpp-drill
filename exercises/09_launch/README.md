# 課題 09: launch を Python / XML / YAML で書く 〔上級〕

公式ガイド
[Using Python, XML, and YAML for ROS 2 launch files](https://docs.ros.org/en/jazzy/How-To-Guides/Launch-file-different-formats.html)
にあるとおり、ROS 2 の launch ファイルは Python / XML / YAML のどれでも書けます。
書式が違っても、内部で作られる「起動するノードの一覧（`LaunchDescription`）」は
同じものになります。

この課題は**採点方法が他と違います**。C++ ではなく `pytest` で採点されます。
`launch/talker_listener.launch.py` / `.xml` / `.yaml` の 3 ファイルを実際に読み込み、
中身の構造（package / executable / namespace / remap）を比較します。

## やること

`launch/` ディレクトリの 3 つのファイルに、**同じ内容**を 3 つの書式で書いてください。
課題01（`drill_01_publisher`）の `talker` と、課題02（`drill_02_subscriber`）の
`listener` を、次の設定で起動します。

| 項目 | talker | listener |
| --- | --- | --- |
| package | `drill_01_publisher` | `drill_02_subscriber` |
| executable | `talker` | `listener` |
| name | `talker` | `listener` |
| namespace | `demo` | `demo` |
| remap | `topic` → `chatter` | `topic` → `chatter` |

- `launch/talker_listener.launch.py`
- `launch/talker_listener.launch.xml`
- `launch/talker_listener.launch.yaml`

3 ファイルとも、ファイル内のコメントに要素名の骨格を書いてあります。値を埋めてください。
3 つとも埋め終わったら、各ファイルの `I AM NOT DONE` 行を削除してください。

## 3 つの書式の比較と使い分け

| 観点 | Python | XML | YAML |
| --- | --- | --- | --- |
| 読みやすさ | プログラムに慣れていれば読みやすいが、量が増えると縦に長くなる | タグの入れ子で構造が見やすい。ROS 1 の launch に近い | インデントで構造が見やすい。一番短く書ける |
| 条件分岐・繰り返し | `if` / `for` がそのまま使える。任意の Python コードを書ける | `IfCondition` / `UnlessCondition` はあるが、ループはできない | XML と同様、条件分岐（`if`/`unless`属性）はあるがループはできない |
| 他ファイルの include | `IncludeLaunchDescription` に好きなロジックを混ぜられる（例: 条件で読むファイルを変える） | `<include file="...">` で宣言的に書ける | `include:` で宣言的に書ける |
| 補完の効き方 | ただの Python なので IDE の補完・型チェックがそのまま効く | スキーマ補完がないと属性名を覚えていないと書けない | XML と同様、覚えていないと書けない |

**使い分けの指針**: 静的な構成（起動するノードの一覧が固定で、値も決め打ち）なら
XML か YAML の方が短く読みやすいです。条件分岐や計算、動的にノードの数を変える
必要があるなら Python 一択です（`if` や `for` が要る時点で XML/YAML は無理をすることになります）。
実務では「大枠は XML/YAML、複雑な部分だけ Python の `OpaqueFunction` で組み立てる」
という混在もよく行われます。

## よく使う要素の書き方

`DeclareLaunchArgument`（起動時に渡せる引数を宣言する）:

| 書式 | 書き方 |
| --- | --- |
| Python | `DeclareLaunchArgument("use_sim_time", default_value="false")` |
| XML | `<arg name="use_sim_time" default="false"/>` |
| YAML | `- arg: {name: use_sim_time, default: "false"}` |

`LaunchConfiguration`（宣言した引数の値を参照する）:

| 書式 | 書き方 |
| --- | --- |
| Python | `LaunchConfiguration("use_sim_time")` を `Node(parameters=[{...}])` などに渡す |
| XML | 属性の値の中で `$(var use_sim_time)` と書く |
| YAML | 属性の値の中で `$(var use_sim_time)` と書く（XML と同じ構文） |

`IncludeLaunchDescription`（他の launch ファイルを読み込む）:

| 書式 | 書き方 |
| --- | --- |
| Python | `IncludeLaunchDescription(PythonLaunchDescriptionSource([FindPackageShare("pkg"), "/launch/other.launch.py"]))` |
| XML | `<include file="$(find-pkg-share pkg)/launch/other.launch.py"/>` |
| YAML | `- include: {file: "$(find-pkg-share pkg)/launch/other.launch.py"}` |

`GroupAction` + `PushRosNamespace`（複数ノードをまとめて 1 つの名前空間に入れる）:

| 書式 | 書き方 |
| --- | --- |
| Python | `GroupAction([PushRosNamespace("demo"), Node(...), Node(...)])` |
| XML | `<group> <push_ros_namespace namespace="demo"/> <node .../> <node .../> </group>` |
| YAML | `- group: {children: [{push_ros_namespace: {namespace: demo}}, {node: {...}}, {node: {...}}]}` |

この課題では `Node` ごとに `namespace="demo"` を直接指定していますが、
ノードが増えてきたら `GroupAction` + `PushRosNamespace` でまとめる方が
書き間違いが減ります。

## 動かしてみる

**手動確認には課題01（`drill_01_publisher`）と課題02（`drill_02_subscriber`）が
実装済みで、かつ `colcon build` 済みである必要があります。** launch ファイル自体は
未完成でも `colcon build --packages-select drill_09_launch` は通りますが、
実際にノードを起動するには 01・02 の実行ファイルが必要です。

```bash
colcon build --packages-select drill_01_publisher drill_02_subscriber drill_09_launch
source install/setup.bash

ros2 launch drill_09_launch talker_listener.launch.py
# .xml / .yaml でも同じように起動できるはず
# ros2 launch drill_09_launch talker_listener.launch.xml
# ros2 launch drill_09_launch talker_listener.launch.yaml
```

別の端末で:

```bash
ros2 node list
# /demo/talker
# /demo/listener

ros2 topic list
# /demo/chatter が見えるはず（remap が効いていれば /demo/topic は出てこない）
```

## つまずきポイント

- XML / YAML の `remap` は `<node>` / `node:` の**中**に入れ子で書きます。
  `<node>` タグの外や、`node:` と同じ階層に置くと属性として認識されません。
- `namespace` は `topic` のようなトピック名の前に `/demo/` を付けるためのものです。
  `name`（ノード名）と役割が違うので混同しないこと。
- YAML はインデントが構造そのものです。タブ文字を混ぜると読み込みエラーになります。
- 3 ファイルとも `I AM NOT DONE` を消し忘れると `./drill list` で未完了のままです。

## テスト

```bash
./drill run 09
```

| テスト | 見ているところ |
| --- | --- |
| `test_python版でtalkerとlistenerの2ノードを起動している` | Python 版に 2 つの `Node`（正しい package/executable）があるか |
| `test_python版のnamespaceとremapがdemoとchatterになっている` | Python 版の namespace と remap の値 |
| `test_xml版がpython版と同じ構造になっている` | XML 版が Python 版と同じ構造か |
| `test_yaml版がpython版と同じ構造になっている` | YAML 版が Python 版と同じ構造か |
| `test_3つの書式がすべて等価である` | 3 書式すべての構造が完全一致するか |

テストは実際にノードのプロセスを起動しません。`launch` / `launch_ros` の
API を使って launch ファイルを読み込み、`package` / `executable` / `namespace` /
`remap` の値だけを文字列として取り出して比較しています。

## 参考

- 公式: [Creating a launch file](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Launch/Creating-Launch-Files.html)
- 公式: [Using Python, XML, and YAML for ROS 2 launch files](https://docs.ros.org/en/jazzy/How-To-Guides/Launch-file-different-formats.html)
- ソース: `/opt/ros/jazzy/lib/python3.12/site-packages/launch/frontend/parser.py`
- ソース: `/opt/ros/jazzy/lib/python3.12/site-packages/launch_ros/actions/node.py`
