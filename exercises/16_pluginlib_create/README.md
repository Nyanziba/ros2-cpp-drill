# 課題 16: プラグインを作って書き出す（pluginlib）〔上級〕

公式チュートリアル
[Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
の「プラグインを作る側」を、速度指令（`cmd_vel`）にかけるフィルタを題材にやります。
読む側（`ClassLoader`）は課題 17・18 で扱います。

課題 15 のコンポーネントは「ノード」を差し替える仕組みでした。pluginlib は
**「ノードの中の部品」を差し替える**仕組みです。基底クラス（`drill::VelocityFilter`）を決めておき、
それを継承したクラスを、名前（文字列）で選んで実行時に読み込みます。

## やること

3 つのフィルタ（プラグイン）のうち、`ClampFilter` の中身と、3 つの「書き出し」を書きます。
`RateLimitFilter` と `DeadbandFilter` の中身は渡してあります。

| クラス | pluginlib の名前 | パラメータ（既定値） | 動き |
| --- | --- | --- | --- |
| `drill::ClampFilter` | `drill/ClampFilter` | `<name>.max_linear` (1.0), `<name>.max_angular` (2.0) | 絶対値を上限で切る |
| `drill::RateLimitFilter` | `drill/RateLimitFilter` | `<name>.max_linear_acceleration` (0.5), `<name>.max_angular_acceleration` (1.0) | 前回の出力からの変化を、加速度 × dt までに制限する |
| `drill::DeadbandFilter` | `drill/DeadbandFilter` | `<name>.linear_threshold` (0.05), `<name>.angular_threshold` (0.1) | 絶対値がしきい値未満なら 0 |

基底クラス（`include/drill/velocity_filter.hpp`）は与えてあります。扱うのは `linear.x` と `angular.z` だけです。
プラグインは**引数なしのコンストラクタで作られる**ので、設定は `initialize(node, name)` で受け取ります
（`name` は YAML の名前空間。パラメータは `<name>.<項目>`）。

TODO は次の 4 か所です。

1. **`src/filters.cpp` の `ClampFilter::filter`。** `linear.x` を `[-max_linear_, +max_linear_]` に、
   `angular.z` を `[-max_angular_, +max_angular_]` に切って返します。負の向きも切ります。
2. **`src/filters.cpp` の末尾で、3 つのクラスを書き出す。**
   ```cpp
   #include <pluginlib/class_list_macros.hpp>
   PLUGINLIB_EXPORT_CLASS(drill::ClampFilter, drill::VelocityFilter)
   ```
   を、3 クラス分です。ライブラリが dlopen されたときに、クラスを作る係が登録されます。
3. **`plugins.xml` に、3 つのクラスを `<class>` で宣言する。** `name`（呼び出し側が使う名前）、
   `type`（C++ のクラス名）、`base_class_type`（基底クラス）を書きます。
4. **`cmake/export_plugins.cmake` で、`plugins.xml` を登録する。**
   `pluginlib_export_plugin_description_file(<基底クラスがあるパッケージ> <XML のパス>)` を呼びます。
   基底クラスはこのパッケージの中にあるので、第 1 引数は自分（`${PROJECT_NAME}`）です。

> CMake の登録だけ `CMakeLists.txt` ではなく `cmake/export_plugins.cmake` に切り出してあります。
> 課題の `CMakeLists.txt` を `templates/` や `solutions/` にも置くと、colcon が別パッケージと
> 取り違えるためです。`CMakeLists.txt` はこのファイルを `include()` しているだけで、編集しません。

## 動かしてみる

```bash
./drill run 16
```

通ったら、書き出した結果を見てみましょう。

```bash
source install/setup.bash
cat install/drill_16_pluginlib_create/share/drill_16_pluginlib_create/plugins.xml
ls install/drill_16_pluginlib_create/lib/
```

`plugins.xml` と共有ライブラリ（`libdrill_16_pluginlib_create_filters.so`）がインストールされていて、
`<library path=...>` がその名前（`lib` と `.so` を除いたもの）を指しています。
テストは、このライブラリにリンクしていません。`pluginlib::ClassLoader` が実行時に
`plugins.xml` を頼りに探して読み込みます。

## つまずきポイント

- 3 つの書き出し（`PLUGINLIB_EXPORT_CLASS`、`plugins.xml`、`pluginlib_export_plugin_description_file`）は
  **どれか 1 つが欠けても、ビルドは通ります。** 失敗するのは実行時（`ClassLoader` が読むとき）です。
  テストの失敗メッセージが、どこを見ればよいかを案内します。
- `plugins.xml` の `name`（`drill/ClampFilter`）と `type`（`drill::ClampFilter`）は別物です。
  呼び出し側は `name` で選び、`type` は pluginlib が C++ のクラスと突き合わせるために使います。
- `<library path=...>` は、`add_library` で作ったライブラリの名前です。名前を変えたら XML も直します。
- プラグインのライブラリは **`SHARED`** です。静的ライブラリでは、実行時に読み込む相手になりません。
- `ClampFilter` の上限を `filter()` に固定値（`1.0`）で書くと、パラメータで変えられないフィルタになります
  （`initialize` が読んでメンバに入れた `max_linear_` を使います）。

## テスト

```bash
./drill run 16
```

| テスト | 見ているところ |
| --- | --- |
| `ThreePluginsAreDeclared` | 3 つの名前が `plugins.xml` から `ClassLoader` に届いているか（`<class>` と CMake の登録の両方） |
| `ThreePluginsCanBeCreated` | 3 つとも実際に作れるか（`PLUGINLIB_EXPORT_CLASS` と、ライブラリの `path`） |
| `ClampFilterCutsAbsoluteValueAtLimits` | 既定の上限（1.0 / 2.0）で、両方向を切れているか。上限の内側は素通しか |
| `ClampFilterUsesParametersUnderInstanceName` | 渡した名前（`limit`）の `limit.max_linear` を使っているか |
| `RateLimitAndDeadbandWorkThroughClassLoader` | 渡してある 2 つが、`ClassLoader` 越しに動くか |

## 参考

- 公式: [Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
- 読み物: [ROS2講習15b: pluginlib](../../docs/ros2/15b_pluginlib.md)
- 次の課題: 17（`ClassLoader` で読み込む）、18（YAML でプラグインの列を組む）
