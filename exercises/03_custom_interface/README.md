# 課題 03: カスタムインターフェースを定義して使う 〔初級〕

公式チュートリアル
[Creating custom msg and srv files](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.html)
の `tutorial_interfaces` パッケージ（`msg/Num.msg` と `srv/AddThreeInts.srv`）を
そのまま定義し、
[Single package: define and use an interface](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Single-Package-Define-And-Use-Interface.html)
の手順で、定義したその場で C++ から使います。

これまでの課題では `std_msgs::msg::String` や `example_interfaces::srv::AddTwoInts`
のような**既製の型**を使っていました。今回はその型自体を自分で作ります。

## やること

3か所を埋めます。

1. `msg/Num.msg` — `int64` 型のフィールド `num` を1つ定義する。
2. `srv/AddThreeInts.srv` — リクエスト側に `int64 a`, `int64 b`, `int64 c`、
   レスポンス側に `int64 sum` を定義する（`---` で区切る）。
3. `src/num_publisher.cpp` — 定義した2つの型を実際に使う `NumNode` を実装する。

| 項目 | 値 |
| --- | --- |
| ノード名 | `num_node` |
| トピック名 | `num` |
| トピックの型 | `drill_03_custom_interface::msg::Num` |
| publish 周期 | 500ms |
| サービス名 | `add_three_ints` |
| サービスの型 | `drill_03_custom_interface::srv::AddThreeInts` |
| サービスの応答 | `response->sum = request->a + request->b + request->c;` |

`NumNode` は「`Num` を publish する仕事」と「`AddThreeInts` サービスを提供する仕事」
の両方を持ちます。公式チュートリアルはこの2つを別々の章・別々の例で示していますが
（`Num` の publish は practice problem の解答、`AddThreeInts` を使うサービスは別章）、
1課題1パッケージ・1課題1概念の方針を崩さないため、この課題では1つのノードに
まとめています。クラス宣言（`include/drill/num_node.hpp`）は与えてあるので、
`NumNode` のコンストラクタと2つのメンバ関数の中身を考えてください。

## `.msg` / `.srv` の書き方

`.msg` は「型 名前」を1行に1つ並べるだけです。

```
int64 num
```

`.srv` は `---` の上がリクエスト、下がレスポンスです。

```
int64 a
int64 b
int64 c
---
int64 sum
```

## なぜインターフェース定義と使う側を同じパッケージに入れるのか

講習資料 [13_カスタムインターフェース](../../../ros2_lecture/13_カスタムインターフェース.md)
の口頭試問Q3にある通り、実務では**インターフェース定義用のパッケージ**
（`tutorial_interfaces` 相当）と、**それを使うノードのパッケージ**を分けるのが
一般的です（型だけ使いたい別パッケージが不要な依存を抱え込まずに済むため）。

この課題ではあえて分けず、公式チュートリアルの
[Single package: define and use an interface](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Single-Package-Define-And-Use-Interface.html)
の構成（1パッケージで完結させる）を採用しています。理由は2つです。

- 練習帳の1課題を1パッケージに収めたい（他の課題と構成を揃えたい）。
- 「同じパッケージ内で生成した型を使うとき特有の CMake の書き方」
  （`rosidl_get_typesupport_target`）を経験してほしい。

複数のノードパッケージから同じ型を使い回したくなったら、口頭試問Q3の理由で
インターフェース専用パッケージに切り出すのが実務での判断です。ここは
講習資料と矛盾しませんが、**この課題自体は公式が示すもう一方の構成**である
ことを理解しておいてください。

### CMakeLists.txt のポイント

```cmake
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Num.msg"
  "srv/AddThreeInts.srv"
)
ament_export_dependencies(rosidl_default_runtime)

# 同じパッケージ内で生成した型を使うときだけ必要な書き方。
# 別パッケージの型なら find_package(<pkg> REQUIRED) だけで済む。
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)

target_link_libraries(drill_03_custom_interface_node "${cpp_typesupport_target}")
```

`rosidl_generate_interfaces` に書いたファイルパスがそのままビルド対象になります。
新しい `.msg` / `.srv` を追加したら、必ずこのリストにも追記してください
（**忘れるとファイルはあるのにビルドされず「型が見つからない」エラーになります**。
講習資料にもある、よくあるハマりどころです）。

## ビルドが通る/落ちるタイミングについて

この課題は少し特殊です。`msg/Num.msg` や `srv/AddThreeInts.srv` が
**コメントだけ**の状態でも、`rosidl_generate_interfaces` 自体はビルドが通ります
（フィールドが1つも無い `.msg` / `.srv` は「空のメッセージ」として合法です。
`std_srvs/srv/Empty` と同じ扱いです）。

一方、この課題の採点用テスト（`test/test_exercise.cpp`、編集禁止）は
`Num::num` や `AddThreeInts::Request::a` に実際に値を代入するコードを含んでいます。
そのため、`msg/Num.msg` に `num` フィールドを書く前は、**テストのビルド自体が
コンパイルエラーで失敗します**。

```
error: no member named 'num' in 'drill_03_custom_interface::msg::Num_<std::allocator<void> >'
```

これは意図的な挙動です。「フィールド名や型を間違えるとコンパイルが通らない」
という、カスタムインターフェース特有の失敗モードをそのまま体験してもらうためです。
エラーメッセージに `num`（またはフィールド名）が出ているので、まず
`msg/Num.msg` / `srv/AddThreeInts.srv` を疑ってください。

## 動かしてみる

テストが通ったら、公式チュートリアルと同じように手で動かせます。

```bash
source install/setup.bash
ros2 run drill_03_custom_interface num_node
```

別の端末で:

```bash
ros2 interface show drill_03_custom_interface/msg/Num
ros2 interface show drill_03_custom_interface/srv/AddThreeInts
ros2 topic echo /num
ros2 service call /add_three_ints drill_03_custom_interface/srv/AddThreeInts "{a: 1, b: 2, c: 3}"
```

## つまずきポイント

- `create_publisher()` / `create_wall_timer()` / `create_service()` の戻り値は
  必ずメンバ変数に代入します。ローカル変数で受けるとコンストラクタを抜けた
  時点で破棄され、何も起きません（01・04と同じ罠です）。
- `rosidl_generate_interfaces` に書いたファイルパスの一覧に、新しい `.msg` /
  `.srv` を足し忘れるとビルドされません（この課題ではすでに書いてあります。
  自分で新しい型を追加する練習をするときのために覚えておいてください）。
- 生成されるヘッダのファイル名は CamelCase から snake_case になります。
  `Num.msg` → `<パッケージ名>/msg/num.hpp`、`AddThreeInts.srv` →
  `<パッケージ名>/srv/add_three_ints.hpp`（`include/drill/num_node.hpp` に
  書いてあるので、写すだけなら気にしなくて大丈夫です）。
- `add_three_ints` はハンドラです。値を `return` するのではなく、引数で渡された
  `response`（`shared_ptr`）のメンバに書き込みます（04課題と同じ）。

## テスト

```bash
./drill run 03
```

| テスト | 見ているところ |
| --- | --- |
| `Num型がint64のnumという1フィールドで定義されている` | `msg/Num.msg` のフィールド名・型（コンパイル自体が検証） |
| `numトピックにNumがpublishされている` | Publisher とタイマが動いているか、`count_++` |
| `add_three_intsサービスを公開している` | `srv/AddThreeInts.srv` の定義、`create_service` を `service_` に入れているか |
| `3つの整数の和を返す` | `response->sum = request->a + request->b + request->c;`、負の数を含む計算 |
| `公式と同じ書式でIncoming_requestログを出している` | `RCLCPP_INFO` の書式（04課題の3引数版） |

## 参考

- 公式: [Creating custom msg and srv files](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.html)
- 公式: [Single package: define and use an interface](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Single-Package-Define-And-Use-Interface.html)
- 講習資料: [13_カスタムインターフェース](../../../ros2_lecture/13_カスタムインターフェース.md)
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
