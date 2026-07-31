# ROS 2 練習帳（ros2-drill）

Rust の [rustlings](https://rustlings.rust-lang.org/) 方式の **ROS 2 / C++ 講習用ドリル**です。
穴埋めのソースを埋めるとテストが合否を判定するので、採点する人が要りません。

```bash
./drill watch
```

保存するたびに再テストされます。読み物（`docs/`）と課題（`exercises/`）は
**章番号まで 1 対 1 で対応**しています。

## 3 つのトラック

| トラック | 読み物 | 課題 | 対象 |
| --- | --- | --- | --- |
| **C++入門編** | [docs/cpp-basics/](docs/cpp-basics/README.md) | `cppb01`〜`cppb10` | `const` や `static` で手が止まる人。全10章 |
| **C++編** | [docs/cpp/](docs/cpp/README.md) | `cpp01`〜`cpp12` | rclcpp を読む準備。全15章 |
| **ROS 2編** | [docs/ros2/](docs/ros2/01_この記事からスタート_ROS2講習ハブ.md) | `01`〜`15` | 全24本 |

**C++入門編と C++編は ROS 2 を使いません。** `ament_cmake` と gtest だけなので、
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

## はじめかた

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
| `./drill read [ID]` | 対応する講習資料の章を開く |
| `./drill solution ID` | 解答例を表示 |
| `./drill reset ID` | 課題を初期状態に戻す |
| `./drill verify` | 全課題を順にテスト |
| `./drill completion [bash\|zsh]` | タブ補完スクリプトを出力 |

ID は接頭辞で区別します。**`cppb` = C++入門編、`cpp` = C++編、数字だけ = ROS 2編**。
`./drill run cppb06` / `./drill run cpp06` / `./drill run 01` のように打ちます。
`./drill run publisher` のような部分一致でも通ります。

タブ補完は `~/.bashrc` に次の 1 行を足してください（zsh は `completion/drill.zsh`）。

```bash
source /path/to/ros2-drill/completion/drill.bash
```

## 読み物をサイトとして読む

`docs/` は Markdown のまま読めますが、MkDocs Material のサイトにもできます。
**章送り・トラックのタブ・日本語の全文検索**が付きます。

```bash
python3 -m venv .venv-docs          # Ubuntu なら先に sudo apt install python3-venv
.venv-docs/bin/pip install -r docs-requirements.txt
.venv-docs/bin/mkdocs serve         # http://127.0.0.1:8000
```

`mkdocs build --strict` でリンク切れと見出しアンカーの不一致がエラーになります。
CI（`.github/workflows/docs.yml`）もこれを回し、成果物を artifact に置きます。

### サイトを GitHub Pages で公開する

`Settings > Pages > Source` を **GitHub Actions** にしてから、
`.github/workflows/docs.yml` の `upload-artifact` ステップを Pages 用に差し替えます。

```yaml
      - uses: actions/upload-pages-artifact@v3
        with:
          path: site/
      - uses: actions/deploy-pages@v4
```

`permissions` に `pages: write` と `id-token: write` を足す必要があります。

## この教材に含まれていないもの

- **C言語編**（マイコン側の C の講習・12章）。特定プロジェクトの通信プロトコル定義を
  題材にしているため、この公開版には含めていません
- ROS 2編の**第4部（自律移動スタックの設計）**。同じ理由です

第3部までで ROS 2 の基礎・パッケージ開発・TF2・URDF・`ros2_control`・センサ統合まで揃います。

## ライセンス

MIT。詳細は [LICENSE](LICENSE) を参照してください。
