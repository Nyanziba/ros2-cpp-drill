# 課題 06: クラスの中でパラメータを使う 〔初級〕

公式チュートリアル
[Using parameters in a class (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Using-Parameters-In-A-Class-CPP.html)
の `MinimalParam` をそのまま書きます。

## やること

`src/minimal_param.cpp` の TODO を埋めてください。仕様は公式チュートリアルと同一です。

| 項目 | 値 |
| --- | --- |
| ノード名 | `minimal_param_node` |
| パラメータ名 | `my_parameter` |
| 型 | 文字列 |
| 既定値 | `"world"` |
| description | `"This parameter is mine!"` |
| 周期 | 1000ms |
| ログ | `Hello <my_parameter の値>!` |

クラス宣言（`include/drill/minimal_param.hpp`）は与えてあります。メンバ変数
`timer_` に何を入れるかと、コンストラクタでのパラメータ宣言、タイマのコールバックの
中身を考えてください。

公式のポイントはタイマのコールバックの最後です。`ros2 param set` で
`my_parameter` を書き換えても、次にコールバックが呼ばれたときに
`"world"` に上書きされて戻ります。パラメータは「一度読んで覚えておく」のではなく
「使うたびに読む」のが原則、というのがこのチュートリアルの主旨です。

## 動かしてみる

テストが通ったら、公式チュートリアルと同じように手で動かせます。

```bash
source install/setup.bash
ros2 run drill_06_parameters minimal_param_node
```

別の端末で:

```bash
ros2 param list
ros2 param describe /minimal_param_node my_parameter
ros2 param get /minimal_param_node my_parameter
ros2 param set /minimal_param_node my_parameter earth
```

`set` の直後は `earth` に変わりますが、1 秒待ってからもう一度 `ros2 param get` する
と `world` に戻っているはずです。ノード自身が毎周期 `set_parameters()` で書き戻して
いるためです。

## つまずきポイント

- `declare_parameter()` の第 3 引数に `ParameterDescriptor` を渡すのを忘れると
  description が空のままになります。
- `create_wall_timer()` の戻り値は必ず `timer_` に代入します。ローカル変数で受けると
  コンストラクタを抜けた時点で破棄され、何も起きません。
- `set_parameters()` に渡すのは `std::vector<rclcpp::Parameter>` です。
  `rclcpp::Parameter` 1 個だけを渡そうとしないこと。
- `std::bind(&MinimalParam::timer_callback, this)` の `this` を忘れないこと。

## 公式との違い

- 公式チュートリアルはクラスと同じファイルに `main()` を書きますが、テストから
  クラスを直接使えるように `src/minimal_param_main.cpp` に分けています。
- 公式チュートリアルはタイマのコールバックをコンストラクタ内のラムダで書きますが、
  テストから呼び出しやすいようにメンバ関数 `timer_callback()` にしています。
  中身は公式と同一です。

## テスト

```bash
./drill run 06
```

| テスト | 見ているところ |
| --- | --- |
| `my_parameterが文字列で既定値worldになっている` | パラメータの型と既定値 |
| `ParameterDescriptorのdescriptionが設定されている` | `declare_parameter()` に渡す description |
| `タイマがHello_worldとログを出している` | タイマが動いていて `get_parameter()` を読んでいるか |
| `ros2_param_setで変えても1秒後にworldへ戻る` | `set_parameters()` で毎周期 `"world"` に戻しているか |

## 参考

- 公式: [Using parameters in a class (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Using-Parameters-In-A-Class-CPP.html)
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
