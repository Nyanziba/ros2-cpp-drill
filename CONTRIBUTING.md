# 貢献のしかた

**English: [CONTRIBUTING.en.md](CONTRIBUTING.en.md)**

この教材への貢献を歓迎します。誤字 1 文字の修正から、新しい章・課題の追加まで、どれも助かります。
直すのが大変なら、**Issue で報告するだけでも十分な貢献です。**
「こういう章や課題が欲しい」という要望も受け付けています。Issue の「要望」テンプレートから送ってください。

これは個人で管理しているプロジェクトです。ROS 2 や C++ の最新の仕様に沿うよう努めていますが、すべてには追いつけません。古くなっている箇所を見つけたら、Issue で教えてください。

貢献していただいたものは、このリポジトリと同じ [MIT ライセンス](LICENSE) で公開されます。
このプロジェクトに参加するすべての人は、[行動規範](CODE_OF_CONDUCT.md)に従ってください。

## 目次

- [報告する（Issue）](#報告するissue)
- [変更を送る（Pull Request）](#変更を送るpull-request)
- [いちばん大事なルール: 出力は実測](#いちばん大事なルール-出力は実測)
- [読み物（docs/）を書く](#読み物docsを書く)
- [課題（exercises/）を作る](#課題exercisesを作る)
- [英語版](#英語版)
- [送る前のチェックリスト](#送る前のチェックリスト)

## 報告する（Issue）

[Issue](https://github.com/Nyanziba/ros2-cpp-drill/issues/new/choose) のテンプレートから選んでください。

| こんなとき | テンプレート |
| --- | --- |
| 載っている出力が、手元で動かした結果と違う | 出力が実測と違う |
| 誤字、説明の誤り、リンク切れ、英訳の誤り | 内容の誤り |
| 課題がビルドできない・テストが通らない、`drill` の不具合 | 課題・drill の不具合 |
| こういう章・課題・トピックが欲しい、古くなっている箇所がある | 要望 |

**出力の食い違いは特に歓迎します。** この教材は「載っている出力はすべて実測」を約束しているので、
食い違いはそのまま誤りです。報告には、動かした環境（OS、`g++ --version`、Docker かどうか）を添えてください。

## 変更を送る（Pull Request）

1. このリポジトリを fork して、`main` からブランチを切ります（例: `fix/c03-output`）。
2. 変更します。**1 つの PR には 1 つの話題だけ**を入れてください（誤字の修正と章の追加を混ぜない）。
3. 下の[チェックリスト](#送る前のチェックリスト)を確かめます。
4. `main` に向けて PR を出します。PR のテンプレートが出るので、項目を埋めてください。

コミットメッセージは `<種類>: <説明>` の形にします。種類は `feat` `fix` `docs` `refactor` `test` `ci` `chore` のどれかです。
説明は日本語でも英語でも構いません。

```
fix: C 03章の実行結果を実測に合わせる
```

### 環境を用意する

課題のビルドには ROS 2 Jazzy（Ubuntu 24.04）が要ります。**Docker を使うのがいちばん確実です。**
手順は [README](README.md) と [はじめかた](docs/はじめかた.md) にあります。

```bash
docker compose build
docker compose run --rm drill ./drill verify     # 全課題をテストする
```

読み物のサイトを手元で確かめるには、次のようにします。

```bash
python3 -m venv .venv-docs
.venv-docs/bin/pip install -r docs-requirements.txt
.venv-docs/bin/mkdocs build --strict                    # 日本語版 → site/
.venv-docs/bin/mkdocs build --strict -f mkdocs.en.yml   # 英語版 → site/en/（必ず日本語版の後）
```

## いちばん大事なルール: 出力は実測

**読み物に載せるコンパイルエラー・警告・実行結果は、すべて実際に動かした出力にしてください。**
予想や記憶で書いた出力、手で書き換えた出力は載せません。

- **環境は Ubuntu 24.04 / g++ 13.3.0 / ROS 2 Jazzy です。** リポジトリの Docker イメージがこの環境です。
  - これまでの出力は x86_64 で取ってあります。Apple Silicon の Mac なら、
    `docker build --platform linux/amd64 -t ros2-drill:jazzy-amd64 .` で x86_64 のイメージを作って測ってください。
    実行結果は arm64 でもほぼ同じですが、`char` の符号など、アーキテクチャで変わる箇所があり得ます。
  - 別の環境（例: macOS の Apple clang）で測ったときは、**本文にその環境を明記**してください
    （デザインパターン編には「Apple clang / arm64 で実測」と明記した箇所があります）。
- **コードを変えたら、出力も測り直します。** コメントを 1 行変えただけでも、
  そのソース行を引用するコンパイルエラーの出力や行番号が変わることがあります。
- **出力を出したコードは、本文に全部載せます。** 本文に無い `main` などを補って測ると、読者が同じ出力を再現できません。
- アドレス値や時刻など実行ごとに変わる値は、測った値のままで構いません。
- 出力の一部だけを載せるときは、省いたことが分かるように `...` などで示します。
- 環境によって結果が変わるもの（データ競合、未定義動作、終了コードなど）は、測った環境と「環境によって変わる」ことを書きます。

### tools/measure.py で測る

出力ブロックの直前には、測り方を書いた印を置きます。`tools/measure.py` がこの印を読んで Docker で動かし、本文と比べます。

```markdown
<!-- measure: filter="grep -o 'error:.*'" -->
```

- 使うファイルは、1 行目に `// ファイル名.cpp` のコメントを書いたコードブロックから取ります。コマンドは直前の `bash` のブロックです。
  どちらも `files=` と `cmd=` で指定できます。抜粋は `filter=`、Apple clang で測るなら `env=clang`、ROS 2 なら `env=ros`、
  動かせないもの（GUI・実機など）は `env=static reason="..."` です。くわしくは `tools/measure.py` の先頭に書いてあります。
- 出力を新しく載せる・コードを変えたら、`python3 tools/measure.py --write <ページ>` で測り直して書き込み、
  `python3 tools/measure.py --check <ページ>` で食い違いが無いことを確かめます。CI（measure.yml）でも同じ検査を回します。

C++ の章のコードには [Compiler Explorer](https://godbolt.org/) のリンク（`▶ ブラウザで実行する`）が付いています。
**コードを変えたら、リンクも作り直してください。** Compiler Explorer でコードを貼り替え、
コンパイラ `x86-64 gcc 13.3`、オプションは本文のコマンドと同じにして、Share の短縮リンクに差し替えます。

## 読み物（docs/）を書く

- 置き場所はトラックごとのディレクトリ（`docs/c/` `docs/cpp-basics/` `docs/cpp/` `docs/ros2/` `docs/patterns/`）です。
  ファイル名は `<番号>_<題>.md` です。
- **新しいページは `mkdocs.yml` の `nav` に足してください。** 足さないとサイトに出ません。
- ページ同士のリンクは、相対パスで `.md` に向けます（例: `[6. const](06_const.md)`）。
- `mkdocs build --strict` は、リンク切れと見出しアンカー（`#...`）の不一致で落ちます。

### 章の型

既存の章は、だいたい次の型で書いてあります。新しい章もこれに合わせてください。

```markdown
# 6. const

> **この章のねらい**: （何ができるようになるか、を 2〜3 行で）

## 6.1 （節）
（本文。節番号は「章.節」）

## 手元で試す

（試すコード）

**予想: （実行する前に、何が起きるかを予想させる問い）**

    g++ -std=c++17 -Wall -Wextra -Wpedantic xxx.cpp -o xxx && ./xxx

[▶ ブラウザで実行する（gcc 13.3）](https://godbolt.org/z/...)

<details markdown="1"><summary>解答（実行結果）</summary>

（実測した出力）

</details>

## つまずきポイント
## 対応する課題
## 参考

---

前章 → [...](...)
次章 → [...](...)
```

- **「予想」の答えは必ず `<details>` で畳みます。** 読者が予想してから開く、という使い方を前提にしているためです。
- 文体は「です・ます」で、短く言い切ります。

## 課題（exercises/）を作る

読み物の章と課題は **1 対 1** で対応させます（例: `docs/cpp-basics/06_const.md` ↔ `exercises/cppb06_const`）。

### ファイルの構成

`<id>` は課題 ID です（例: `cppb06_const`）。

```
exercises/<id>/
├── CMakeLists.txt          # project(drill_<id>)。-Wall -Wextra -Wpedantic を付ける
├── package.xml             # <name>drill_<id></name>
├── README.md               # 課題文（やること・動かしかた・つまずきポイント・テスト・参考）
├── include/drill/...       # ヘッダ
├── src/...                 # 受講者が編集するファイル（先頭に // I AM NOT DONE）
└── test/test_exercise.cpp  # 採点用のテスト（受講者は編集しない）
templates/<id>/...          # 受講者が編集するファイルの初期状態（./drill reset で戻す先）
solutions/<id>/...          # 解答例（./drill solution で表示する）
```

- 受講者が編集するファイルの先頭には `// I AM NOT DONE` を置きます。`drill` は、
  このマーカーが全部消えたら「完了」とみなします。
- `templates/<id>/` には、`exercises/<id>/` の編集対象ファイルと**同じ内容**を置きます。
- ROS 2 を使うテストは、共通ヘルパ [`tools/drill_harness.hpp`](tools/drill_harness.hpp) を使います。
- テストの失敗メッセージは、**次に何を確かめればよいか**が分かるように書きます
  （例: 「`create_publisher<std_msgs::msg::String>("topic", 10)` を `publisher_` に入れましたか？」）。
- テスト名（`TEST(Suite, 名前)` や pytest の `def test_...`）は、日英共通の**英語の識別子**にします（例: `SwapsTwoVariables`）。
- 失敗メッセージは `drill::localized("日本語", "English")`（[`tools/drill_i18n.hpp`](tools/drill_i18n.hpp)）で囲みます。`DRILL_LANG` で日英が切り替わります。
- `static_assert` はコンパイル時に出るので切り替えられません。メッセージは `"日本語 / English"` の 1 つの文字列にします。

### exercises.json に登録する

`exercises.json` の `exercises` 配列に 1 件足します。並び順が `./drill list` の順番になります。

| 項目 | 内容 |
| --- | --- |
| `id` / `package` | 課題 ID と、パッケージ名（`drill_<id>`） |
| `level` / `level_note` | トラック名と、その説明 |
| `title` / `chapter` | 課題名と、対応する章（`./drill list` に出る） |
| `kind` | `gtest`（通常）/ `pytest`（launch や YAML）/ `gtest_mutation`（受講者がテストを書く課題） |
| `sources` | 受講者が編集するファイル（課題ディレクトリからの相対パス） |
| `hints` | `./drill hint` で出すヒント。弱いものから順に |
| `lecture` | 対応する章（`title` と `path`） |
| `docs` | 公式ドキュメントへのリンク（`./drill doc`） |

英語版の項目（`title_en` `hints_en` `lecture_en` など）は [英語版](#英語版) を見てください。

### 確かめる

**未解答のままでは落ち、解答例を当てれば通ること**の両方を確かめます。

```bash
docker compose run --rm drill ./drill run <id>     # 未解答: 落ちること
cp -r solutions/<id>/. exercises/<id>/              # 解答例を当てる
docker compose run --rm drill ./drill run <id>     # 通ること
docker compose run --rm drill ./drill reset <id>   # 元に戻す
```

`exercises/` には未解答の状態でコミットしてください（`// I AM NOT DONE` が残った状態）。

全課題をまとめて確かめるには `docker compose run --rm drill python3 tools/verify_exercises.py` を使います（`--track cppb` や `--id cppb06` で絞れます）。課題ごとに「未解答で落ちる」「解答例で通る」を見て、終わると `templates/` の内容に戻します。CI（`.github/workflows/exercises.yml`）も同じスクリプトを回します。

## 英語版

英語版の読み物は `docs-en/`、課題文は `exercises/<id>/README.en.md`、
`exercises.json` では `*_en` の項目です。**日本語版が正で、英語版はその翻訳です。**

- 日本語版を直したら、**できれば同じ PR で英語版も直してください。** 規則・ファイル名の対応表・用語集は
  [TRANSLATING.md](TRANSLATING.md) にあります。
- 英語が難しければ、日本語版だけ直して PR の本文に「英語版は未対応」と書いてください。メンテナが追います。
- 日本語版だけが変わった PR には、CI（translation-drift）が警告を出します。警告だけで落ちはしませんが、「英語版は未対応」と書いていないと要約で促されます。
- 英語版でコードを英訳したときも、出力は[実測](#いちばん大事なルール-出力は実測)し直します。

## 送る前のチェックリスト

PR のテンプレートにも同じ項目があります。

- [ ] 載せた出力は、すべて実際に動かした結果（環境が既定と違えば本文に明記）
- [ ] コードを変えたなら、出力と Compiler Explorer のリンクも作り直した
- [ ] `mkdocs build --strict`（読み物を変えたなら英語版の `-f mkdocs.en.yml` も）が通る
- [ ] 課題を変えたなら、未解答で落ち、解答例で通ることを確かめた
- [ ] 英語版も直した（または PR に「英語版は未対応」と書いた）
- [ ] `python3 tools/check_docs.py` が通る（日英のページ構造、課題データ、テンプレートの整合を機械で確かめます）
- [ ] 1 つの PR に 1 つの話題だけ
