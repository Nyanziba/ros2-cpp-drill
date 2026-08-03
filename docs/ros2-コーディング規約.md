# ROS 2 のコーディング規約と言語バージョン（まとめ）

公式ドキュメント
[Code style and language versions](https://docs.ros.org/en/jazzy/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html)
（原文: `ros2_documentation/source/The-ROS2-Project/Contributing/Code-Style-Language-Versions.rst`、jazzy ブランチ）
の要点を日本語でまとめたものです。**これは要約なので、判断に迷う場面では必ず原文を確認してください。**

原文が 403 で読めないときは raw を直接読めます。

```
https://raw.githubusercontent.com/ros2/ros2_documentation/jazzy/source/The-ROS2-Project/Contributing/Code-Style-Language-Versions.rst
```

## 対象バージョン

| 言語 | ターゲット |
| --- | --- |
| C | C99 |
| C++ | **C++17** |
| Python | Python 3 |
| CMake | 3.14.4 以上（REP 2000） |

## C++ — Google C++ Style Guide + ROS 2 の改変

ベースは Google C++ Style Guide。そこに ROS 2 独自の変更が入っています。
**変更点を知らないと Google スタイルのまま書いてレビューで指摘される**ので、ここが実質の本体です。

### 書式

| 項目 | ROS 2 のルール | 補足 |
| --- | --- | --- |
| 行長 | **100 文字**まで | Google の 80 から緩めている |
| ヘッダ拡張子 | **`.hpp`** | C と C++ をツールが区別できるようにするため |
| 実装拡張子 | **`.cpp`** | 同上 |
| ポインタ | `char * c;` | `char* c;` ではない。複数宣言時に破綻しないため |
| テンプレートの入れ子 | `set<list<string>>` | 間に空白を入れない |
| `public:` などの前 | 空白 0（推奨）または 2 | インデントは 2 の倍数を保つ |
| インデント | 空白 2、タブ禁止 | |
| 波かっこ | **常に付ける** | `if` / `else` / `do` / `while` / `for` が 1 行でも省略しない |
| 波かっこの位置 | 関数・クラス・enum・struct の定義は改行して開く。制御構文は同じ行（cuddle） | 条件が折り返す場合は制御構文でも改行して開く |
| 関数呼び出しの折り返し | 開きかっこの位置で折り返し、継続行は 2 スペース | |

### 命名

| 対象 | ルール |
| --- | --- |
| グローバル変数 | 小文字＋アンダースコア、`g_` を付ける |
| 定数 | Google から逸脱。歴史的経緯で `snake_case` / `PascalCase` / `UPPER_CASE` が混在 |
| 関数・メソッド | `CamelCase`（Google 式）と `snake_case`（標準ライブラリ式）の**どちらも可**。ROS 2 のコアは `snake_case`。新規プロジェクトは関連する既存プロジェクトに合わせる |

### 言語機能の可否

| 機能 | ROS 2 の立場 |
| --- | --- |
| **ラムダ / `std::function` / `std::bind`** | **制限なし**（"No restrictions"） |
| 例外 | 可。ユーザ向け API では慣用的。ただし**デストラクタでは避ける**。C でラップする API では避けることを検討 |
| クラスメンバの公開範囲 | 「全メンバを private に」という Google の要求は外す。既定は private、必要なものだけ public |
| Boost | **どうしても必要な場合を除き避ける** |

ラムダと `std::bind` に公式の優劣がない点は重要です。「bind は古いから直すべき」は公式ルールでは
ありません。どちらを選ぶかの判断材料は
[7. ラムダと `std::bind`](cpp/07_ラムダとstd_bind.md) にまとめてあります。

### コメント

- ドキュメント用（クラス・関数）: `///` と `/** */`
- 実装メモ: `//`
- 理由: Doxygen と Sphinx で拾えるため

### リンタと静的解析

| ツール | ament ラッパ |
| --- | --- |
| Google `cpplint.py` | `ament_cpplint` |
| `uncrustify` | `ament_uncrustify` |
| clang-format | `ament_clang_format` |
| `cppcheck` | `ament_cppcheck` |

コンパイラフラグは **`-Wall -Wextra -Wpedantic`** を付けることが前提になっています
（この練習帳の各課題の `CMakeLists.txt` もこの 3 つを有効にしています）。

## C — PEP 7 + 改変

- C99 を対象（`//` と `/* */` の両方が使えること、C99 が十分普及していることが理由）
- C++ 形式の `//` コメントを許可
- 比較でリテラルを左に置くのは任意（`0 == ret`。誤代入の防止）
- Python モジュール以外では、すべてに `Py_` を付ける規則は適用しない。CamelCase のパッケージ名を使う
- スタイルチェックには `pep7` を使用

## Python — PEP 8 + 明確化

- 行長は **100 文字**まで許容
- クォートは**シングルクォート優先**（エスケープが必要な場合を除く）
- 継続行は hanging indent を優先
- import は 1 行 1 つを優先
- リンタは `pycodestyle` / `ament_pycodestyle`

## CMake

- コマンド名は小文字（`find_package`。`FIND_PACKAGE` ではない）
- 識別子は `snake_case`
- `else()` や `end...()` の引数は空にする
- `(` の前に空白を置かない
- インデントは空白 2、タブ禁止
- 複数行マクロで桁揃えのインデントをしない（空白 2 のみ）
- マクロより「関数 + `set(... PARENT_SCOPE)`」を優先
- マクロ内のローカル変数は `_` などの接頭辞を付ける

## ドキュメント（Markdown / reStructuredText）

- **1 文 1 行**（sentence per line）。行末の余分な空白は置かない
- 見出しの前後に空行を 1 つ
- コードブロックの前後に空行、言語指定を書く
- RST の見出し記号は Sphinx の階層に従う（`#`, `*`, `=`, `-`, `^`, `"`）
- Markdown の見出しは ATX 形式（`#` 1〜6 個、後ろに空白）

## この練習帳との対応

この練習帳のコードは上記に沿えるよう次のようにしています。

- 拡張子は `.hpp` / `.cpp`、インデントは空白 2、行長は 100 以内
- 全課題の `CMakeLists.txt` で `-Wall -Wextra -Wpedantic` を有効化
- 関数・メソッド名は `snake_case`（ROS 2 コアに合わせる）
- 公式チュートリアルのコードと字面を合わせることを優先しているため、
  コールバックはメンバ関数 + `std::bind` の形が多い（公式にはラムダ版もあり、どちらも規約上可）

## 関連

- [rclcpp の設計思想](rclcpp-の設計思想.md) — なぜその API 設計なのか
- [7. ラムダと `std::bind`](cpp/07_ラムダとstd_bind.md) — 既存コードを直すときの判断基準
- 原文で確認すべき隣接ドキュメント: `Developer-Guide.rst`（プログラミング規約全般）、
  `Quality-Guide.rst`（品質レベル）、REP 2000（各ディストロの依存バージョン）
