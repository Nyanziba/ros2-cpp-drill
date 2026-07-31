# 課題 08: パラメータを YAML で管理する 〔上級〕

公式ドキュメント
[Understanding parameters](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Parameters/Understanding-ROS2-Parameters.html)
と
[About parameters](https://docs.ros.org/en/jazzy/Concepts/Basic/About-Parameters.html)
にある「パラメータ YAML ファイル」を自分で書きます。

課題06・07 はコード側からパラメータを扱いましたが、今回はコードを一切書きません。
`config/params.yaml` と `launch/param_demo.launch.py` の 2 ファイルだけを書きます。
採点も C++ の gtest ではなく、実際にノードを起動して確認する **pytest** です
（`src/param_echo.cpp` は完成済みで編集しません）。

## やること

`config/params.yaml` と `launch/param_demo.launch.py` の TODO を埋めてください。

`param_echo` というノードに、次の 4 つのパラメータを YAML で設定します。

| パラメータ名 | 型 | 値 |
| --- | --- | --- |
| `my_parameter` | 文字列 | `bonjour` |
| `an_int_param` | 整数 | `7` |
| `a_double_param` | 浮動小数 | `1.5` |
| `a_string_list` | 文字列リスト | `["alpha", "beta"]` |

`launch/param_demo.launch.py` はこの YAML を読み込ませて `param_echo` を起動する
launch ファイルです。

## パラメータ YAML の3段構造

パラメータ YAML はどれも次の3段構造です。ここを間違える人が多いところです。

```yaml
<ノード名>:
  ros__parameters:
    <パラメータ名>: <値>
```

- 1段目はノード名です。`ros2 node list` で出てくる名前をそのまま書きます。
- 2段目は必ず `ros__parameters` という固定のキーです。
  **アンダースコアが2本**であることに注意してください
  （`ros_parameters` ではありません。1本のミスがとても多いです）。
- 3段目にパラメータ名と値を並べます。

### 名前空間つきノード

ノードに名前空間がある場合は、1段目を `/ns/node_name` のように書きます。

```yaml
/my_ns/param_echo:
  ros__parameters:
    my_parameter: bonjour
```

### 全ノード共通のワイルドカード `/**`

1段目をノード名の代わりに `/**` にすると、そのファイルを読み込んだ**すべてのノード**
に同じパラメータが適用されます（個別のノード名指定がある場合はそちらが優先されます）。
今回の課題ではワイルドカードではなく `param_echo` という具体的なノード名を使ってください。

## YAML の型がそのままパラメータの型になる

YAML に書いた値の型が、そのままパラメータの型になります。

| YAML の書き方 | パラメータの型 |
| --- | --- |
| `7` | 整数 |
| `7.0` | 浮動小数 |
| `"7"` | 文字列 |
| `[1, 2]` | 整数の配列 |

**型が合わないと、ノード側の `declare_parameter()` が例外を投げてノードが落ちます。**
`my_parameter` を `bonjour`（クォートなし）と書いても文字列として解釈されますが、
`an_int_param` を `"7"` のようにクォートしてしまうと文字列になり、
`param_echo` 側では `int64_t` として宣言しているため起動時に例外になります。
`a_double_param` は `1.5` のように小数点を書かないと浮動小数になりません
（`1` と書くと整数になってしまいます）。

## 使い方は3通り

同じ YAML ファイルを、次の3つの方法で読み込ませることができます。

1. コマンドラインから直接:
   ```bash
   ros2 run drill_08_params_yaml param_echo --ros-args --params-file config/params.yaml
   ```
2. launch ファイルの `parameters=[...]` から:
   ```python
   Node(
       package='drill_08_params_yaml',
       executable='param_echo',
       parameters=[params_file],
   )
   ```
3. 起動中のノードに `ros2 param load` で後から読み込ませる:
   ```bash
   ros2 param load /param_echo config/params.yaml
   ```

## 今の値を YAML に書き出す

逆に、起動中のノードが今持っている値を YAML として書き出すこともできます。

```bash
ros2 param dump /param_echo
```

3段構造そのままの YAML が標準出力に出てきます。手で YAML を書くときの
お手本としても使えます。

## 動かしてみる

テストが通ったら、手で動かして確認できます。

```bash
source install/setup.bash
ros2 run drill_08_params_yaml param_echo --ros-args --params-file config/params.yaml
```

`param_echo` は4行ログを出してすぐ終了します。

```
[INFO] [...] [param_echo]: my_parameter=bonjour
[INFO] [...] [param_echo]: an_int_param=7
[INFO] [...] [param_echo]: a_double_param=1.5
[INFO] [...] [param_echo]: a_string_list=[alpha,beta]
```

launch 経由でも同じことができます。

```bash
ros2 launch drill_08_params_yaml param_demo.launch.py
```

## つまずきポイント

- `ros__parameters` のアンダースコアは**2本**です。1本にすると
  「そのキーがない」という扱いになり、パラメータが1つも読み込まれません。
- `an_int_param: 7` のように、数値をクォートで囲まないこと。
  `"7"` と書くと文字列になり、ノード側の型と合わずに例外になります。
- `a_double_param` は `1.5` のように小数点まで書くこと。`1` と書くと整数になります。
- `get_package_share_directory()` はインストール後の `share/` ディレクトリを見ます。
  `config/params.yaml` を編集しても `colcon build` していないと launch 側には
  古い内容（または存在しないファイル）が渡ります。

## テスト

```bash
./drill run 08
```

| テスト | 見ているところ |
| --- | --- |
| `testパラメータYAMLが構文として読み込める` | `config/params.yaml` がYAMLとして構文エラーなく読めるか |
| `test_3段構造になっている_ノード名からros__parametersまで` | `param_echo` → `ros__parameters` の階層になっているか |
| `test_4つのパラメータが正しい型と値になっている` | 4つのパラメータの型と値が仕様どおりか |
| `testEndToEndでparam_echoが正しい値をログに出す` | 実際に `param_echo` を起動し、ログに正しい値が出るか |
| `test_launchファイルがparam_echoをparams_yaml付きで起動する` | launch が `param_echo` を `config/params.yaml` 付きで起動しているか |

## 参考

- 公式: [Understanding parameters](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Parameters/Understanding-ROS2-Parameters.html)
- 公式: [About parameters](https://docs.ros.org/en/jazzy/Concepts/Basic/About-Parameters.html)
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
