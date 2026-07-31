# ROS2講習10: ワークスペースとcolcon

## はじめに

この記事を終えると、ROS2のワークスペースの構造を理解し、`colcon`でパッケージをビルドして、自分でビルドしたノードを実行できるようになります。

前提は[09_launchとros2_bag](09_launchとros2_bag.md)までを読み終えていることです。ここまではapt入りの既製パッケージ（turtlesimなど）を使ってきましたが、これ以降は自分やチームで書いたコードをビルドして使う場面が増えていきます。その土台がワークスペースです。

## 講習目標

- ワークスペースの`src`/`build`/`install`/`log`の役割を説明できる
- `colcon build`でパッケージをビルドし、`--symlink-install`と`--packages-select`を使い分けられる
- underlayとoverlayの関係を理解し、sourceする順序を間違えない
- `rosdep`で依存パッケージを解決できる

## 講習として使う場合

### 準備物

- Ubuntu 24.04 + ROS 2 Jazzy Jaliscoがセットアップ済みの環境（[02_環境構築](02_環境構築.md)完了）
- `python3-colcon-common-extensions`（未インストールなら`sudo apt install python3-colcon-common-extensions`）
- `python3-rosdep`（`sudo apt install python3-rosdep`。初回のみ`sudo rosdep init && rosdep update`が必要）
- gitでクローンできるネットワーク環境

### 時間配分の目安

- ワークスペースの構造説明: 10分
- ワークスペース作成〜colcon buildの演習: 15分
- underlay/overlayとsourceの順序: 10分
- examplesリポジトリのビルドと実行課題: 20分
- 口頭試問: 5分

### 口頭試問

**Q1. `colcon build`を実行した直後にできる`build`、`install`、`log`はそれぞれ何のディレクトリですか。**

模範解答: `build`はビルド時の中間ファイル（CMakeのキャッシュやオブジェクトファイルなど）を置く作業ディレクトリ。`install`は実際に使う実行ファイルやライブラリ、`setup.bash`などが配置される、いわば「完成品置き場」。`log`はビルドログで、エラーの原因を調べるときに`log/latest_build/`以下を見る。実行時に必要なのは`install`だけで、`build`と`log`は消してもビルドし直せば復元できる。

**Q2. ビルドがおかしくなったので`build`ディレクトリを消したいと思っています。何に注意すべきですか。**

模範解答: `build`だけでなく`install`も一緒に消してから`colcon build`し直すのが安全。CMakeのキャッシュが`build`にしか残っていない中途半端な削除だと、古い`install`の内容と新しいビルドが噛み合わずに原因不明のエラーになることがある。迷ったら`rm -rf build install log`してクリーンビルドするのが一番早い。ただし他のパッケージも巻き込んで全部ビルドし直しになるので、時間があるときにやる。

**Q3. 新しいターミナルを開いたら、まず何をsourceすべきですか。順序も含めて答えてください。**

模範解答: まず`/opt/ros/jazzy/setup.bash`（underlay、ROS2本体）をsourceし、その後に自分のワークスペースの`install/setup.bash`（overlay）をsourceする。overlay側の`setup.bash`はunderlayが読み込まれていることを前提にしているため、順序を逆にするとエラーになったり、ROS2本体のコマンドが見つからなくなったりする。`~/.bashrc`にunderlayのsourceを書いておき、ワークスペースのoverlayは使うたびに手でsourceする運用が一般的。

## 本文

### 学習内容：ワークスペースとは何か

学習内容：ROS2のワークスペースの構造を理解する。

準備：特になし。

内容：

ワークスペースとは、複数のROS2パッケージをまとめてビルドするための作業場所です。1つのパッケージだけを単体でビルドすることはほとんどなく、依存関係のあるパッケージ群をまとめて`colcon`に渡すのが基本の使い方になります。

まずワークスペースを作ります。

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws
```

このディレクトリだけ覚えておけば十分です。`colcon build`は`src`の中身を見て、その他の`build`・`install`・`log`は実行するとその場に自動で作られます。

| ディレクトリ | 役割 |
|---|---|
| `src` | パッケージのソースコードを置く場所。gitでcloneしたりファイルを書いたりするのはここだけ |
| `build` | ビルド時の中間ファイル。CMakeのキャッシュやオブジェクトファイル |
| `install` | ビルドの完成品。実行ファイル、ライブラリ、`setup.bash` |
| `log` | ビルドログ。エラー原因を追うときに見る |

自分が触るのは`src`だけです。他の3つは`colcon`が管理する領域なので、手で中身をいじる必要はありません。逆に言うと、`build`と`install`と`log`は消してもビルドし直せば元に戻るので、消すことを怖がらなくていい領域です。

### 学習内容：colcon buildの基本

学習内容：`colcon build`でワークスペースをビルドする。

準備：`~/ros2_ws/src`が存在すること。

内容：

`src`に何もパッケージがない状態でも、`colcon build`はワークスペースの初期状態（空の`build`/`install`/`log`）を作ってくれます。

```bash
cd ~/ros2_ws
colcon build
```

実務で毎回使うのは次のオプション付きの形です。

```bash
colcon build --symlink-install
```

`--symlink-install`を付けると、`install`以下のファイルの多くが実体コピーではなくシンボリックリンクになります。Pythonのノードやlaunchファイル、パラメータのyamlなど、C++のようにコンパイルを挟まないファイルを編集したときに、この形式だとビルドし直さずにすぐ反映されます。C++の実行ファイル自体は再コンパイルが必要ですが、それ以外の部分の手戻りが減るので、開発中は基本的にこのオプションを付けてビルドしてください。

```bash
colcon build --packages-select <パッケージ名>
```

ワークスペースに複数のパッケージがあるとき、変更したパッケージだけをビルドしたいことがあります。`--packages-select`はビルド対象を絞り込むオプションで、パッケージ数が増えてビルド時間が延びてきたら重宝します。依存関係にあるパッケージまでまとめてビルドしたい場合は`--packages-up-to`を使いますが、まずは`--packages-select`だけ覚えれば十分です。

ビルドが終わったら`ls install`で完成物を確認してみましょう。パッケージ名のディレクトリが並んでいるはずです。

### 学習内容：underlayとoverlay、sourceの順序

学習内容：ROS2本体（underlay）と自分のワークスペース（overlay）の関係を理解する。

準備：`colcon build`が一度成功していること。

内容：

これまでの記事で新しいターミナルを開くたびに何気なく打っていた次のコマンドの意味を、ここで整理します。

```bash
source /opt/ros/jazzy/setup.bash
```

これはROS2本体（apt でインストールされている部分）を有効化するコマンドです。この土台のことをunderlayと呼びます。そして自分が`colcon build`したワークスペースを有効化するには、`install/setup.bash`をsourceします。

```bash
source ~/ros2_ws/install/setup.bash
```

このワークスペース側をoverlayと呼びます。overlayはunderlayの上に「重ねる」ものなので、sourceする順序は必ずunderlay→overlayです。逆にすると、overlay側の`setup.bash`が前提にしているROS2本体の環境変数（`AMENT_PREFIX_PATH`など）が設定されておらず、`ros2`コマンド自体が見つからなくなったり、パッケージが見つからないというエラーになったりします。

```bash
# 正しい順序
source /opt/ros/jazzy/setup.bash
source ~/ros2_ws/install/setup.bash

# 間違い（overlayを先にsourceしている）
source ~/ros2_ws/install/setup.bash
source /opt/ros/jazzy/setup.bash
```

underlayは`~/.bashrc`に書いておけば新しいターミナルで自動的にsourceされますが、overlayは使うたびに手で打つのが基本です。ワークスペースを複数使い分けている場合、`.bashrc`にoverlayまで書いてしまうと「どのワークスペースが有効か」がわかりにくくなるので、あえて手で打つ運用を勧めます。

もう1つ、詰まりやすいのが**同じターミナルでビルドとsourceを混ぜること**です。`colcon build`を実行したターミナルで直後に`source install/setup.bash`しても一見動きますが、次に同じターミナルで`colcon build`を再実行すると、すでにoverlayがsourceされた状態でビルドが走ることになり、環境が汚れた状態でのビルドになってしまう場合があります。基本の運用は「ビルド用ターミナル」と「実行用ターミナル」を分けることです。ビルドは何もsourceしていない（あるいはunderlayだけの）ターミナルで行い、ビルドが終わったら別のターミナルでoverlayをsourceして実行する、という2枚構成に慣れてください。

> コラム: overlayの中でさらにoverlayを重ねることもできます。あるワークスペースをsourceした状態で別のワークスペースをビルド・sourceすると、後者が前者に依存パッケージを探しに行きます。大きなプロジェクトを複数のワークスペースに分割する場合に使う構成ですが、最初のうちは1つのワークスペースで完結させたほうが混乱しません。

### 学習内容：examplesリポジトリをビルドして実行する

学習内容：外部のROS2パッケージをワークスペースに取り込んでビルドする。

準備：`~/ros2_ws/src`が存在し、underlayをsource済みであること。

内容：

自分でノードを書く前に、ROS2公式のexamplesリポジトリを使ってビルドの流れを体験します。

```bash
cd ~/ros2_ws/src
git clone -b jazzy https://github.com/ros2/examples.git
cd ~/ros2_ws
```

`src`の中にクローンしただけで、ビルドはまだしていません。依存パッケージが揃っているかを確認するために`rosdep`を使います。

```bash
rosdep install --from-paths src --ignore-src -r -y
```

`--from-paths src`は`src`以下のパッケージの`package.xml`を全部見て依存関係を調べるという意味です。`--ignore-src`は「`src`の中にあるパッケージ自身は依存として無視する」（自分自身を自分の依存だと誤認しないようにする）指定で、`-y`は確認プロンプトを自動でyesにするオプションです。examplesリポジトリはROS2本体が提供する型やライブラリしか使わないので、多くの環境ではここで新規インストールは発生しませんが、この手順を通しておくと、後で複数のパッケージを含む外部リポジトリをビルドするときに同じコマンドが使えます。

ビルドします。

```bash
colcon build --symlink-install --packages-select examples_rclcpp_minimal_publisher
```

examplesリポジトリには複数のパッケージが含まれていますが、`--packages-select`で最小の例に絞ってビルドしています。ビルドが終わったらoverlayをsourceして実行します。

```bash
source install/setup.bash
ros2 run examples_rclcpp_minimal_publisher publisher_member_function
```

`Publishing: "Hello, world! 0"`のような行が1秒おきに流れ続ければ成功です。別のターミナルでunderlay→overlayの順にsourceして、`ros2 topic list`や`ros2 topic echo /topic`で流れているメッセージを確認してみましょう。

**練習問題**: `examples_rclcpp_minimal_subscriber`パッケージも同じ手順でビルドし、publisherと同時に実行して、subscriber側のターミナルに受信ログが出ることを確認してください。

<details><summary>解答</summary>

```bash
colcon build --symlink-install --packages-select examples_rclcpp_minimal_subscriber
source install/setup.bash
ros2 run examples_rclcpp_minimal_subscriber subscriber_member_function
```

publisherを実行しているターミナルとは別のターミナルでこれを実行すると、subscriber側に`I heard: "Hello, world! N"`のようなログが流れます。`ros2 topic info /topic`でpublisher countとsubscriber countがそれぞれ1になっていることも確認しておくと、[05_トピック](05_トピック.md)で学んだ内容とつながります。

</details>

複数のパッケージを`vcs`ツールで管理する場合、`.repos`ファイルの`version`にはSHAもブランチ名も書けます。特定の提出物のSHAを直接書いており、上流が動いても勝手に追従しない構成もあります。一方、ブランチ名を書いた場合は`vcs import`を叩いた日によって中身が変わります。**`.repos`を読むときはSHAかブランチ名かを必ず確認してください。** SHAで固定しておけば、他人の環境で再現しないという事故を減らせます。

## 発展

`colcon build`はデフォルトでワークスペース内の全パッケージを並列にビルドしようとします。パッケージ数が増えて手元のマシンが重くなってきたら、`--parallel-workers`で並列数を絞ることができます。また、`colcon test`でパッケージに書かれたテストを一括実行できますが、テストの書き方自体は別の話になるので、ここでは名前だけ知っておいてください。


## おわりに

ワークスペースの構造（`src`/`build`/`install`/`log`）とunderlay/overlayの関係は、ROS2で開発する上でずっと使う基礎知識です。ビルドがおかしいと感じたら、まず`build`と`install`を消してクリーンビルドする、sourceの順序を確認する、の2つを最初に疑ってください。わからなければ先輩に聞きましょう。

次は[11_C++でpub_subを書く](11_C++でpub_subを書く.md)で、今日ビルドしたexamplesと同じ構造のノードを自分で書きます。

## 資料

- [ROS 2 Documentation: Jazzy — Creating a workspace](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Creating-A-Workspace/Creating-A-Workspace.html)
- [ROS 2 Documentation: Jazzy — Using colcon to build packages](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Colcon-Tutorial.html)
- [ros2/examples (jazzy branch)](https://github.com/ros2/examples/tree/jazzy)
- [05_トピック](05_トピック.md)
- [09_launchとros2_bag](09_launchとros2_bag.md)
- [11_C++でpub_subを書く](11_C++でpub_subを書く.md)
