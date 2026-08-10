# ROS 2 練習帳（ros2-drill）

**📖 読み物はここで読めます → <https://nyanziba.github.io/ros2-cpp-drill/>**
（章送り・トラックのタブ・日本語の全文検索つき。インストール不要）

**コード例はブラウザでそのまま動かせます。** C++ の章のコード例には
Compiler Explorer へのリンクが付いているので、`g++` が無くても読み進められます。
「**予想: …**」の答えは畳んであるので、**予想してから開いてください。**

Rust の [rustlings](https://rustlings.rust-lang.org/) 方式の **ROS 2 / C++ 講習用ドリル**です。
穴埋めのソースを埋めるとテストが合否を判定するので、採点する人が要りません。

```bash
./drill watch
```

保存するたびに再テストされます。読み物（`docs/`）は 3 トラック計 49 章で、
課題（`exercises/`）の 35 問は**そのうち演習を設けた章と章番号で対応**しています。

## 4 つのトラック

| トラック | 読み物 | 課題 | 対象 |
| --- | --- | --- | --- |
| **C++入門編** | [docs/cpp-basics/](docs/cpp-basics/README.md) | `cppb01`〜`cppb10` | `const` や `static` で手が止まる人。全10章 |
| **C++編** | [docs/cpp/](docs/cpp/README.md) | `cpp01`〜`cpp12` | rclcpp を読む準備。全15章 |
| **ROS 2編** | [docs/ros2/](docs/ros2/01_この記事からスタート_ROS2講習ハブ.md) | `01`〜`15` | 全24本 |
| **デザインパターン編** | [docs/patterns/](docs/patterns/README.md) | `dp01`〜`dp23` | 部内ライブラリを設計する人。結城本の輪読と並走。全23章 |

**デザインパターン編は他のトラックと独立しています。** 順番の依存はありません。

**C++入門編・C++編・デザインパターン編は ROS 2 を使いません。** `ament_cmake` と gtest だけなので、
1 課題 2〜3 秒でビルドできます。

rclcpp は `shared_ptr`・ラムダ・`std::move`・テンプレートに強く依存しています。
そこが曖昧なまま ROS 2 を始めると、「ROS を学んでいるのか C++ の文法と戦っているのか」が
分からなくなります。だから C++ を先に片付ける構成にしています。

全体像は [教材の全体像](docs/README.md) にあります。

## 掲載しているコードと出力について

**この教材に載っているコンパイルエラー・実行結果は、すべて実際に走らせた出力です。**
予想と実測が食い違った箇所は実測に合わせてあります。
環境は Ubuntu 24.04 / g++ 13.3.0 / ROS 2 Jazzy です。

各章の「手元で試す」は**予想してから実行する**ことを前提に書いています。
外したところがその人の穴なので、飛ばさないでください。

## 動作環境

- ROS 2 Jazzy（`/opt/ros/jazzy`）
- Ubuntu 24.04 / g++ 13 / CMake 3.28
- Python 3（ランナー用。追加ライブラリは不要）

**Ubuntu 以外なら Docker が使えます。** 次の節のとおりです。

## Ubuntu 以外で動かす（Docker）

ROS 2 Jazzy は Ubuntu 24.04 にしか公式パッケージがありません。
macOS でも Windows でも他のディストリでも、コンテナの中身を Ubuntu 24.04 に
揃えれば同じように動きます。arm64（Apple Silicon）でも動きます。

```bash
docker compose build                          # 初回だけ（10〜20 分）
docker compose run --rm drill ./drill list
docker compose run --rm drill ./drill watch cppb01
docker compose run --rm drill                 # bash に入る
```

**ソースはホスト側をそのまま見ています。** 編集は普段のエディタでどうぞ。
保存すれば `watch` が拾います（`drill` は mtime を見るだけなので、
macOS や Windows のバインドマウントでも取りこぼしません）。

**実行ビットが落ちる環境（Windows の NTFS 上に置いた場合など）では**
`./drill` の代わりに `python3 drill list` と打ってください。

ビルド生成物はホストの `build/` `install/` `log/` とは分けています
（名前付きボリューム）。CMake は絶対パスとコンパイラのパスをキャッシュに
焼くので、native ビルドと混ざると壊れるからです。消すときは
`docker compose down -v` です。

**Linux で UID が 1000 でない場合だけ**、先に `.env` を置いてください。
置かないとコンテナの生成物が root 所有になり、ホストのエディタで保存できません。

```bash
printf 'DRILL_UID=%s\nDRILL_GID=%s\n' "$(id -u)" "$(id -g)" > .env
```

`UID` という名前は使えません。bash の組み込み変数で export されていないので、
compose からは見えず常に既定値になります。
macOS と Windows の Docker Desktop は所有権を勝手に合わせるので不要です。

### GUI（turtlesim / rqt / RViz）

課題は全て gtest / pytest なので、**GUI 無しで全課題を最後まで通せます。**
GUI が要るのは読み物のほう（ROS 2編で turtlesim を動かす章）です。

```bash
docker compose build --build-arg WITH_GUI=1     # イメージが 1GB 以上太ります
docker compose --profile x11 run --rm drill-gui ros2 run turtlesim turtlesim_node
```

ホスト側で X の許可が要ります。

| ホスト | やること |
| --- | --- |
| Linux | `xhost +local:docker` |
| macOS | XQuartz を入れて「ネットワーク・クライアントを許可」→ `xhost +localhost`、`DISPLAY=host.docker.internal:0` |
| Windows (WSL2) | WSLg があればそのまま |

CI（`.github/workflows/docker.yml`）が毎回イメージを組んで、**未解答の課題が
落ちること**と**解答を当てた課題が通ること**の両方を確かめています。

## はじめかた

Ubuntu 24.04 に ROS 2 Jazzy が入っている場合です。それ以外は上の Docker へ。

```bash
source /opt/ros/jazzy/setup.bash   # 未 source でも drill が自動で探します
./drill list                       # 課題一覧と進捗
./drill watch                      # 最初の未完了課題を監視しながら進める
```

課題のソースを開いて `TODO` を埋め、保存します。テストが通ったら
ファイル先頭付近の次の行を消すと次の課題に進みます。

```cpp
// I AM NOT DONE
```

## コマンド

| コマンド | 説明 |
| --- | --- |
| `./drill list` | 課題一覧と進捗 |
| `./drill run [ID]` | ビルド＋テスト（ID 省略時は次の未完了課題） |
| `./drill watch [ID]` | 保存を検知して自動で再テスト |
| `./drill hint ID` | ヒント（コードの形まで見せます） |
| `./drill doc ID` | 対応する公式ドキュメントの URL |
| `./drill read [ID]` | 対応する講習資料の章を開く（`--web` でブラウザ） |
| `./drill solution ID` | 解答例を表示 |
| `./drill reset ID` | 課題を初期状態に戻す |
| `./drill verify` | 全課題を順にテスト |
| `./drill completion [bash\|zsh]` | タブ補完スクリプトを出力 |

### 資料をブラウザで読む

`./drill read` は既定で `less` に流します。長い章はサイトのほうが読みやすいので、
`--web` を付けるとブラウザで開きます。

```bash
./drill read --web cppb06     # 既定では公開サイトの該当ページを開きます
./drill read --build cppb06   # 手元にサイトを建ててから開く（オフライン用）
```

開く先の決め方は次の順です。

| 順 | 行き先 |
| --- | --- |
| 1 | `DRILL_DOCS_URL`（自分でサイトを建てている場合） |
| 2 | 手元の `site/`（`file://` で開く） |
| 3 | <https://nyanziba.github.io/ros2-cpp-drill/> |

`--build` は `mkdocs` が無ければ `.venv-docs/` に用意してから建てます。

**Docker の中ではブラウザを開けません。** `site/` はバインドマウントの上に
できるので、`site/cpp-basics/06_const.html` をホスト側のブラウザで開いてください
（そのパスを表示します）。

ID は接頭辞で区別します。**`cppb` = C++入門編、`cpp` = C++編、数字だけ = ROS 2編**。
`./drill run cppb06` / `./drill run cpp06` / `./drill run 01` のように打ちます。
`./drill run publisher` のような部分一致でも通ります。

タブ補完は `~/.bashrc` に次の 1 行を足してください（zsh は `completion/drill.zsh`）。

```bash
source /path/to/ros2-drill/completion/drill.bash
```

## エディタ（VS Code）

`.vscode/` を同梱しています。開くだけで IntelliSense が効きます。
無いと `rclcpp` などが全部赤線になり、こう出ます。

```
#include errors detected. Please update your includePath.
Squiggles are disabled for this translation unit.
```

**ROS 2 のヘッダは `/opt/ros/jazzy/include/<パッケージ名>/<パッケージ名>/...` と
1 段深いところにあります。** `#include "rclcpp/rclcpp.hpp"` を解決するには
`/opt/ros/jazzy/include` ではなくその下の各パッケージのディレクトリが要るので、
`**` で再帰的に拾っています。

**自作の `.msg` / `.srv` から生えるヘッダだけは、一度ビルドするまで赤線が残ります。**
`install/` にしか存在しないためです。`./drill run 03` を通せば消えます。

### Ubuntu 以外なら Dev Container

`/opt/ros/jazzy` がホストに無いと IntelliSense は効きません。
`.devcontainer/` を同梱しているので、**「Dev Containers: Reopen in Container」**
を実行すればコンテナの中で開けます。`compose.yaml` をそのまま使うので、
ドリルを走らせる環境と同じものが開きます。

## 読み物をサイトとして読む

### 公開サイト

**<https://nyanziba.github.io/ros2-cpp-drill/>**

`main` に push されるたびに `.github/workflows/docs.yml` が自動で配信します。
入口はここです。

| | |
| --- | --- |
| 全体像（入口） | <https://nyanziba.github.io/ros2-cpp-drill/> |
| C++入門編 | <https://nyanziba.github.io/ros2-cpp-drill/cpp-basics/> |
| C++編 | <https://nyanziba.github.io/ros2-cpp-drill/cpp/> |
| ROS 2編 | <https://nyanziba.github.io/ros2-cpp-drill/ros2/01_この記事からスタート_ROS2講習ハブ.html> |

### 手元でサイトを立てる

オフラインで読みたいときや、書き換えて確認したいときはこちら。

```bash
python3 -m venv .venv-docs          # Ubuntu なら先に sudo apt install python3-venv
.venv-docs/bin/pip install -r docs-requirements.txt
.venv-docs/bin/mkdocs serve         # http://127.0.0.1:8000
```

`mkdocs build --strict` でリンク切れと見出しアンカーの不一致がエラーになります。
CI もこれを回すので、リンクを壊した push は落ちます。
ビルド済みのサイトは Actions の `docs-site` artifact からも落とせます。

### fork して自分のサイトにする

`Settings > Pages > Source` を **GitHub Actions** にして、`mkdocs.yml` の
`site_url` を自分のものに書き換えれば、そのまま配信されます。

## この教材が扱う範囲

**C++ と ROS 2 の基礎から、rclcpp のコードを読み書きできるところまで**です。
ROS 2編は第3部まで、つまり基礎・パッケージ開発・TF2・URDF・`ros2_control`・
センサ統合までを扱います。

自律移動スタックそのものの設計（プランナやコントローラの実装）は扱いません。
そこは題材が個別のロボットに強く依存するので、この教材の外です。

## ライセンス

MIT。詳細は [LICENSE](LICENSE) を参照してください。
