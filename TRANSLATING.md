# 英語版の作りかた（Translation guide）

英語版は日本語版の**翻訳**です。内容を足したり削ったりしません。
日本語版を直したら、対応する英語版も同じ PR で直してください。

## 置き場所

| 日本語（正） | 英語 |
| --- | --- |
| `docs/` | `docs-en/`（`mkdocs.en.yml` で `site/en/` に出力） |
| `README.md` | `README.en.md` |
| `exercises/<id>/README.md` | `exercises/<id>/README.en.md` |
| `exercises.json` の `title` / `hints` など | 同じ課題に `title_en` / `hints_en` / `level_en` / `level_note_en` / `chapter_en` / `lecture_en` |
| `drill` の出力 | `DRILL_LANG=en ./drill ...` で英語になる |

ソースコード・テスト・テンプレート・解答のコメントは**日本語のまま**です（二重管理を避けるため）。
テスト名は日英共通の英語の識別子です。テストの失敗メッセージは `drill::localized("日本語", "English")`
（Python は `localized`）で書き、`DRILL_LANG` で切り替わります。`static_assert` はコンパイル時に出るので、
`"日本語 / English"` の 1 つの文字列にします。

## ファイル名

英語版のファイル名は ASCII の snake_case にします。番号は日本語版と揃えます。

| 日本語 | 英語 |
| --- | --- |
| `docs/README.md` | `docs-en/README.md` |
| `docs/はじめかた.md` | `docs-en/getting-started.md` |
| `docs/c/README.md` | `docs-en/c/README.md` |
| `docs/c/01_分割コンパイルとヘッダガード.md` | `docs-en/c/01_separate_compilation_and_header_guards.md` |
| `docs/c/02_固定幅整数と暗黙変換.md` | `docs-en/c/02_fixed_width_integers_and_implicit_conversions.md` |
| `docs/c/03_ポインタ1.md` | `docs-en/c/03_pointers_1.md` |
| `docs/c/04_ポインタ2.md` | `docs-en/c/04_pointers_2.md` |
| `docs/c/05_文字列.md` | `docs-en/c/05_strings.md` |
| `docs/c/06_構造体とアラインメント.md` | `docs-en/c/06_structs_and_alignment.md` |
| `docs/c/07_ビット演算とレジスタ操作.md` | `docs-en/c/07_bit_operations_and_registers.md` |
| `docs/c/08_手動メモリ管理.md` | `docs-en/c/08_manual_memory_management.md` |
| `docs/c/09_関数ポインタ.md` | `docs-en/c/09_function_pointers.md` |
| `docs/c/10_volatileと割り込み安全性.md` | `docs-en/c/10_volatile_and_interrupt_safety.md` |
| `docs/c/11_エンディアンとシリアライズ.md` | `docs-en/c/11_endianness_and_serialization.md` |
| `docs/c/12_プリプロセッサとデバッグ.md` | `docs-en/c/12_preprocessor_and_debugging.md` |
| `docs/cpp-basics/README.md` | `docs-en/cpp-basics/README.md` |
| `docs/cpp-basics/01_宣言を読む.md` | `docs-en/cpp-basics/01_reading_declarations.md` |
| `docs/cpp-basics/02_スコープと寿命.md` | `docs-en/cpp-basics/02_scope_and_lifetime.md` |
| `docs/cpp-basics/03_参照.md` | `docs-en/cpp-basics/03_references.md` |
| `docs/cpp-basics/04_ポインタ1.md` | `docs-en/cpp-basics/04_pointers_1.md` |
| `docs/cpp-basics/05_ポインタ2.md` | `docs-en/cpp-basics/05_pointers_2.md` |
| `docs/cpp-basics/06_const.md` | `docs-en/cpp-basics/06_const.md` |
| `docs/cpp-basics/07_static.md` | `docs-en/cpp-basics/07_static.md` |
| `docs/cpp-basics/08_その他の修飾子.md` | `docs-en/cpp-basics/08_other_qualifiers.md` |
| `docs/cpp-basics/09_値のセマンティクス.md` | `docs-en/cpp-basics/09_value_semantics.md` |
| `docs/cpp-basics/10_ヘッダとプロジェクト構成.md` | `docs-en/cpp-basics/10_headers_and_project_layout.md` |
| `docs/cpp/README.md` | `docs-en/cpp/README.md` |
| `docs/cpp/01_ビルドとリンクの仕組み.md` | `docs-en/cpp/01_how_build_and_link_work.md` |
| `docs/cpp/02_クラスと初期化.md` | `docs-en/cpp/02_classes_and_initialization.md` |
| `docs/cpp/03_継承.md` | `docs-en/cpp/03_inheritance.md` |
| `docs/cpp/04_参照とconst.md` | `docs-en/cpp/04_references_and_const.md` |
| `docs/cpp/05_ムーブと所有権.md` | `docs-en/cpp/05_move_and_ownership.md` |
| `docs/cpp/06_スマートポインタ.md` | `docs-en/cpp/06_smart_pointers.md` |
| `docs/cpp/07_ラムダとstd_bind.md` | `docs-en/cpp/07_lambdas_and_std_bind.md` |
| `docs/cpp/08_autoと型推論.md` | `docs-en/cpp/08_auto_and_type_deduction.md` |
| `docs/cpp/09_テンプレートの読み方.md` | `docs-en/cpp/09_reading_templates.md` |
| `docs/cpp/10_演算子オーバーロード.md` | `docs-en/cpp/10_operator_overloading.md` |
| `docs/cpp/11_標準ライブラリの道具箱.md` | `docs-en/cpp/11_standard_library_toolbox.md` |
| `docs/cpp/12_chronoと時間.md` | `docs-en/cpp/12_chrono_and_time.md` |
| `docs/cpp/13_並行性の最小限.md` | `docs-en/cpp/13_minimal_concurrency.md` |
| `docs/cpp/14_エラーの扱い.md` | `docs-en/cpp/14_error_handling.md` |
| `docs/cpp/15_チェックリスト.md` | `docs-en/cpp/15_checklist.md` |
| `docs/ros2/01_この記事からスタート_ROS2講習ハブ.md` | `docs-en/ros2/01_start_here_course_hub.md` |
| `docs/ros2/02_環境構築.md` | `docs-en/ros2/02_environment_setup.md` |
| `docs/ros2/03_turtlesimとrqtで感覚を掴む.md` | `docs-en/ros2/03_getting_a_feel_with_turtlesim_and_rqt.md` |
| `docs/ros2/04_ノード.md` | `docs-en/ros2/04_nodes.md` |
| `docs/ros2/05_トピック.md` | `docs-en/ros2/05_topics.md` |
| `docs/ros2/05b_DDSとdiscoveryとROS_DOMAIN_ID.md` | `docs-en/ros2/05b_dds_discovery_and_ros_domain_id.md` |
| `docs/ros2/06_サービス.md` | `docs-en/ros2/06_services.md` |
| `docs/ros2/07_パラメータ.md` | `docs-en/ros2/07_parameters.md` |
| `docs/ros2/08_アクション.md` | `docs-en/ros2/08_actions.md` |
| `docs/ros2/09_launchとros2_bag.md` | `docs-en/ros2/09_launch_and_ros2_bag.md` |
| `docs/ros2/10_ワークスペースとcolcon.md` | `docs-en/ros2/10_workspaces_and_colcon.md` |
| `docs/ros2/11_C++でpub_subを書く.md` | `docs-en/ros2/11_writing_pub_sub_in_cpp.md` |
| `docs/ros2/12_Pythonでpub_subを書く.md` | `docs-en/ros2/12_writing_pub_sub_in_python.md` |
| `docs/ros2/13_カスタムインターフェース.md` | `docs-en/ros2/13_custom_interfaces.md` |
| `docs/ros2/14_サービスの実装.md` | `docs-en/ros2/14_implementing_services.md` |
| `docs/ros2/15_パラメータとlaunchの実践.md` | `docs-en/ros2/15_parameters_and_launch_in_practice.md` |
| `docs/ros2/15b_pluginlib.md` | `docs-en/ros2/15b_pluginlib.md` |
| `docs/ros2/16_アクションサーバの実装.md` | `docs-en/ros2/16_implementing_an_action_server.md` |
| `docs/ros2/17_テストとデバッグ.md` | `docs-en/ros2/17_testing_and_debugging.md` |
| `docs/ros2/18_TF2と座標系.md` | `docs-en/ros2/18_tf2_and_coordinate_frames.md` |
| `docs/ros2/19_URDFの書き方.md` | `docs-en/ros2/19_writing_urdf.md` |
| `docs/ros2/20_ros2_control概要.md` | `docs-en/ros2/20_ros2_control_overview.md` |
| `docs/ros2/21_センサ統合.md` | `docs-en/ros2/21_sensor_integration.md` |
| `docs/ros2/22_自己位置推定の考え方.md` | `docs-en/ros2/22_thinking_about_localization.md` |
| `docs/patterns/README.md` | `docs-en/patterns/README.md` |
| `docs/patterns/00_使う前に.md` | `docs-en/patterns/00_before_you_use_them.md` |
| `docs/patterns/01_Iterator.md` | `docs-en/patterns/01_Iterator.md` |
| `docs/patterns/02_Adapter.md` | `docs-en/patterns/02_Adapter.md` |
| `docs/patterns/03_TemplateMethod.md` | `docs-en/patterns/03_TemplateMethod.md` |
| `docs/patterns/04_FactoryMethod.md` | `docs-en/patterns/04_FactoryMethod.md` |
| `docs/patterns/05_Singleton.md` | `docs-en/patterns/05_Singleton.md` |
| `docs/patterns/06_Prototype.md` | `docs-en/patterns/06_Prototype.md` |
| `docs/patterns/07_Builder.md` | `docs-en/patterns/07_Builder.md` |
| `docs/patterns/08_AbstractFactory.md` | `docs-en/patterns/08_AbstractFactory.md` |
| `docs/patterns/09_Bridge.md` | `docs-en/patterns/09_Bridge.md` |
| `docs/patterns/10_Strategy.md` | `docs-en/patterns/10_Strategy.md` |
| `docs/patterns/11_Composite.md` | `docs-en/patterns/11_Composite.md` |
| `docs/patterns/12_Decorator.md` | `docs-en/patterns/12_Decorator.md` |
| `docs/patterns/13_Visitor.md` | `docs-en/patterns/13_Visitor.md` |
| `docs/patterns/14_ChainOfResponsibility.md` | `docs-en/patterns/14_ChainOfResponsibility.md` |
| `docs/patterns/15_Facade.md` | `docs-en/patterns/15_Facade.md` |
| `docs/patterns/16_Mediator.md` | `docs-en/patterns/16_Mediator.md` |
| `docs/patterns/17_Observer.md` | `docs-en/patterns/17_Observer.md` |
| `docs/patterns/18_Memento.md` | `docs-en/patterns/18_Memento.md` |
| `docs/patterns/19_State.md` | `docs-en/patterns/19_State.md` |
| `docs/patterns/20_Flyweight.md` | `docs-en/patterns/20_Flyweight.md` |
| `docs/patterns/21_Proxy.md` | `docs-en/patterns/21_Proxy.md` |
| `docs/patterns/22_Command.md` | `docs-en/patterns/22_Command.md` |
| `docs/patterns/23_Interpreter.md` | `docs-en/patterns/23_Interpreter.md` |
| `docs/rclcpp-の設計思想.md` | `docs-en/rclcpp_design_philosophy.md` |
| `docs/ros2-コーディング規約.md` | `docs-en/ros2_coding_conventions.md` |
| `docs/発表資料.md` | `docs-en/slides.md` |

新しい章を足したら、この表と `mkdocs.en.yml` の nav に足してください。

## リンク

- **docs/ 内のページへのリンク**は、上の表の英語版の相対パスに書き換えます（全ページに英語版があります）。
- **同じファイル内のアンカー**は、英語の見出しから作り直します。
  slug は「小文字・空白は `-`・記号は落とす」（GitHub と同じ）です。
  例: `## 1.1 Reading declarations right to left` → `#11-reading-declarations-right-to-left`
- **別ファイルへのアンカー**（`other.md#...`）は、相手の英語見出しが分からないので
  **日本語のアンカーのまま残します**。全ページがそろった後の仕上げで機械的に直します。
- 英語版の無いファイル（`docs/発表資料/` の PDF・画像など）は、日本語サイトの絶対 URL にします。
  例: `https://nyanziba.github.io/ros2-cpp-drill/発表資料/slides-10min.pdf`
- `exercises/` へのリンク（`../../exercises/...`）は、課題の `README.en.md` に向けます。
- 外部リンク（cppreference・ROS 公式など）は**そのまま**です。
- Compiler Explorer（godbolt）のリンクは、英訳したコードで**作り直します**。コンパイラとオプションは日本語版のリンクと同じにします
  （`https://godbolt.org/api/shortlinkinfo/<id>` で元の設定を取り、`source` だけ差し替えて `https://godbolt.org/api/shortener` に送る）。

## 書きかた

- 日本語版の「です・ます」で短く言い切る調子を、英語では平易な短文にします。
  受講者は英語が母語とは限りません。難しい語彙や慣用句は避けます。
- 太字・表・見出し番号（`## 1.1`）・admonition（`!!! note`）・`<details>` の構造は**変えません**。
- **コードブロック**: コードはそのまま。日本語のコメントと日本語の文字列リテラルだけ英訳します。
- **出力は実測です。** 出力ブロック（コンパイルエラー・実行結果・コマンドの出力）は翻訳時には
  **一字も変えません**。コードを英訳したことで出力が変わるもの（文字列の出力、
  ソース行を引用するコンパイルエラー）は、後の「実測パス」で原文と同じ環境で動かし直して差し替えます。
  手で書き換えて実測のふりをしてはいけません。
  - 原文の環境: 既定は Ubuntu 24.04 / g++ 13.3.0（Docker イメージ `ros2-drill:jazzy-amd64`、linux/amd64）。
    章が「Apple clang / arm64」と明記している箇所は macOS の clang で測ります。
  - アドレス値など実行ごとに変わる値は、測り直した値に置き換えます。
  - どうしても再実行できないもの（GUI・実機など）は、出力を原文のまま残し、
    出力中の日本語は訳さずに残します。
- 宣言の読み下し（「p は int へのポインタ」）は英語の読み下し（"p is a pointer to int"）にします。
  右から左に読む手順が英語でも成り立つように訳してください。
- 課題 README のテスト名（例: `AllowsCallsOnConstObject`）は日英共通の英語の識別子なので、日英どちらの README にも
  そのまま書きます（訳を添えません）。
- テストの失敗メッセージは `drill::localized("日本語", "English")` で書きます。英語は用語集に従い、平易で短くします。
  `static_assert` のメッセージは `"日本語 / English"` の 1 つの文字列にし、README などで引用するときも同じ文字列にします。

## 用語集

| 日本語 | 英語 |
| --- | --- |
| 練習帳 / ドリル | drill |
| 課題 | exercise |
| 読み物 / 講習資料 | reading / lecture notes |
| 章 | chapter |
| トラック（C++入門編 など） | track |
| C言語編 / C++入門編 / C++編 / ROS 2編 / デザインパターン編 | C track / C++ Basics / C++ / ROS 2 / Design Patterns |
| この章のねらい | Goal of this chapter |
| 対応する課題 | Matching exercise |
| 手元で試す | Try it yourself |
| 予想: … | Predict: … |
| 解答（答え合わせ） | Answer (check yourself) |
| 解答（実行結果） | Answer (actual output) |
| ▶ ブラウザで実行する | ▶ Run in your browser |
| つまずきポイント | Common pitfalls |
| やること | What to do |
| 動かしてみる | Run it |
| 見ているところ | What it checks |
| 参考 | References |
| 寿命 | lifetime |
| 再指向 | reseat |
| 逆参照 / 間接参照 | dereference |
| 翻訳単位 | translation unit |
| 所有権 | ownership |
| 値のセマンティクス | value semantics |
| 結城浩『増補改訂版 Java言語で学ぶデザインパターン入門』 | Hiroshi Yuki, *Learning Design Patterns in Java* (revised and expanded edition, in Japanese)（英題は著者の英語プロフィール <https://hyuki.com/info/eprofile> による） |
| 結城本 | Yuki's book（書名を出すときは *Learning Design Patterns in Java*） |
