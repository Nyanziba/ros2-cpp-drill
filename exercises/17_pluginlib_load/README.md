# 課題 17: ClassLoader でプラグインを読み込む（pluginlib）〔上級〕

公式チュートリアル
[Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
の「読み込む側」です。課題 16 で作った 3 つのフィルタ（`ClampFilter` / `RateLimitFilter` / `DeadbandFilter`）は
完成した形で渡してあります。受講者は、**パラメータで 1 つ選んで読み込むノード `FilterHost`** を書きます。

## やること

`include/drill/filter_host.hpp` と `src/filter_host.cpp` の TODO を埋めてください。

| 項目 | 値 |
| --- | --- |
| クラス名 | `FilterHost`（`rclcpp::Node`。名前は `filter_host`） |
| パラメータ | `filter_type`（文字列。既定は空）。pluginlib の名前（例: `drill/ClampFilter`） |
| プラグインに渡す名前 | `kFilterInstanceName`（`"filter"`）。パラメータは `filter.max_linear` のようになる |
| ClassLoader | `pluginlib::ClassLoader<drill::VelocityFilter>("drill_17_pluginlib_load", "drill::VelocityFilter")` |

公開するメソッドは宣言してあります。メンバ（ClassLoader と選んだフィルタ）は、ヘッダに自分で宣言します。

1. **メンバを宣言する。** `ClassLoader` と、選んだフィルタ（`std::shared_ptr<drill::VelocityFilter>`）です。
   **宣言の順番に意味があります**（下の「つまずきポイント」）。
2. **`configure()`**: `filter_type` を読み、`createSharedInstance` でプラグインを作り、
   `initialize(shared_from_this(), kFilterInstanceName)` を呼んでメンバに入れます。
3. **名前が見つからないとき**（`pluginlib::PluginlibException`）は、受けて、`RCLCPP_ERROR` で
   読み込めなかった名前と `e.what()` を出し、**フィルタは空のまま（通過）にします。** ノードは落としません。
4. **`apply(command, dt_seconds)`**: フィルタがあれば通した結果を、空なら `command` をそのまま返します。
5. **`available_filter_names()`**: 使える名前の一覧（`getDeclaredClasses()`）を返します。

`configure()` が別にあるのは、`initialize` がノードを `rclcpp::Node::SharedPtr` で受け取るためです。
`shared_from_this()` は、`std::make_shared` が終わって自分が `shared_ptr` に入った後でないと使えません
（コンストラクタの中で呼ぶと例外になります）。`ClassLoader` だけはコンストラクタの初期化子リストで作ります。

## 動かしてみる

```bash
./drill run 17
source install/setup.bash
ros2 run drill_17_pluginlib_load filter_host_node --ros-args -p filter_type:=drill/ClampFilter
ros2 run drill_17_pluginlib_load filter_host_node --ros-args -p filter_type:=drill/Typo
```

2 つ目は存在しない名前です。落ちずに、読み込めなかった名前と使える名前の一覧のエラーが出れば成功です
（Ctrl-C で止めます）。

## つまずきポイント

- **`ClassLoader` を、フィルタより先に宣言してください。** メンバは宣言の逆順に壊れます。
  フィルタを先に宣言すると `ClassLoader` が先に壊れ、フィルタが残っているのに `ClassLoader` が消えます。
  class_loader が `SEVERE WARNING!!! Attempting to unload library while objects created by this loader exist in the heap!`
  という警告を標準エラーに出し、未定義の動作になりえます。テストは標準エラーを捕まえて、この警告が出ないことを確かめます。
- `createSharedInstance` が投げるのは `pluginlib::PluginlibException` です。`std::exception` で
  受けても動きますが、読み込みの失敗だけを受けるために `PluginlibException` で受けます。
- 失敗したときは、**メンバのフィルタを空にして**ください。作りかけのフィルタ（`initialize` に失敗したもの）を
  入れたままにすると、通過にならず、半端に動きます。
- `initialize(shared_from_this(), ...)` をコンストラクタの中に書くと、`bad_weak_ptr` で落ちます。

## テスト

```bash
./drill run 17
```

| テスト | 見ているところ |
| --- | --- |
| `SelectsFilterByFilterTypeParameter` | `filter_type` の名前で 3 つのうち 1 つが選ばれ、`filter.<項目>` で設定され、状態を保つか |
| `FallsBackToPassThroughWhenNameIsUnknown` | 存在しない名前で落ちず、通過になり、ログに名前が出るか |
| `PassesThroughWhenFilterTypeIsNotGiven` | `filter_type` を指定しなくても、落ちずに通過するか |
| `ListsAvailableFilterNames` | `available_filter_names()` に 3 つの名前が入っているか |
| `HostWithLiveFilterIsDestroyedWithoutClassLoaderWarning` | フィルタを持ったまま壊しても class_loader の警告が出ないか（`ClassLoader` を先に宣言したか） |

## 参考

- 公式: [Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
- 読み物: [ROS2講習15b: pluginlib](../../docs/ros2/15b_pluginlib.md)
