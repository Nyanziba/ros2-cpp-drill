# このファイルは編集しません（採点用）。
"""課題08: パラメータを YAML で管理する — 採点用 pytest。

観点は5つ:
  1. config/params.yaml が YAML として読める
  2. 3段構造になっている（<ノード名>: -> ros__parameters: -> <パラメータ名>: <値>）
  3. 4つのパラメータが正しい型・値で書かれている
  4. end-to-end: ビルド済みの param_echo を実際に起動し、ログの出力を確認する
  5. launch/param_demo.launch.py が param_echo を正しいパラメータで起動する

失敗したときは、何が期待値で何が実際の値か、次に何をすればよいかが分かるように
メッセージを書いてあります。
"""
import importlib.util
import os
import subprocess
import sys
from pathlib import Path

import pytest
import yaml

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools"))
from drill_i18n import localized  # noqa: E402

PACKAGE_NAME = "drill_08_params_yaml"
NODE_NAME = "param_echo"

# .../exercises/13_params_yaml/test/test_exercise.py
TEST_DIR = Path(__file__).resolve().parent
EXERCISE_DIR = TEST_DIR.parent
EXERCISES_ROOT = EXERCISE_DIR.parent
REPO_ROOT = EXERCISES_ROOT.parent

YAML_PATH = EXERCISE_DIR / "config" / "params.yaml"
LAUNCH_PATH = EXERCISE_DIR / "launch" / "param_demo.launch.py"

EXPECTED = {
    "my_parameter": "bonjour",
    "an_int_param": 7,
    "a_double_param": 1.5,
    "a_string_list": ["alpha", "beta"],
}


# ---------------------------------------------------------------------------
# ヘルパ: config/params.yaml の読み込みと構造チェック
# ---------------------------------------------------------------------------


def _load_yaml():
    """params.yaml を読み込む。構文エラーなら分かりやすく落とす。"""
    if not YAML_PATH.exists():
        pytest.fail(localized(
            f"{YAML_PATH} が見つかりません。ファイルを作成してください。",
            f"{YAML_PATH} was not found. Please create the file.",
        ))
    text = YAML_PATH.read_text(encoding="utf-8")
    try:
        return yaml.safe_load(text)
    except yaml.YAMLError as e:
        pytest.fail(localized(
            f"{YAML_PATH} の YAML の構文エラーです: {e}\n"
            "  インデントは半角スペースのみ（タブ禁止）、"
            "\":\" の後には半角スペースが必要です。",
            f"YAML syntax error in {YAML_PATH}: {e}\n"
            "  Indent with spaces only (no tabs), "
            "and put a space after \":\".",
        ))


def _get_ros_parameters(root, *, source=YAML_PATH):
    """root から <ノード名>.ros__parameters の dict を取り出す。

    3段構造になっていない場合は、原因を具体的に指摘して pytest.fail する。
    """
    if root is None:
        pytest.fail(localized(
            f"{source} の中身が空です（コメントだけ、または \"I AM NOT DONE\" のままかもしれません）。\n"
            "  次の3段構造で書いてください:\n"
            f"    {NODE_NAME}:\n      ros__parameters:\n        my_parameter: ...\n",
            f"{source} is empty (it may contain only comments, or still say \"I AM NOT DONE\").\n"
            "  Write it in this 3-level structure:\n"
            f"    {NODE_NAME}:\n      ros__parameters:\n        my_parameter: ...\n",
        ))
    if not isinstance(root, dict):
        pytest.fail(localized(
            f"{source} のトップレベルは辞書（マッピング）である必要があります。"
            f"実際の型: {type(root).__name__} / 実際の値: {root!r}",
            f"The top level of {source} must be a dictionary (mapping). "
            f"Actual type: {type(root).__name__} / actual value: {root!r}",
        ))

    if NODE_NAME not in root:
        wildcard = "/**"
        if wildcard in root:
            pytest.fail(localized(
                f"トップレベルのキーが \"{wildcard}\"（全ノード共通ワイルドカード）になっています。\n"
                f"  この課題ではノード名そのもの \"{NODE_NAME}\" をキーにしてください。\n"
                f"  期待するキー: {NODE_NAME} / 実際のキー: {list(root.keys())}",
                f"The top-level key is \"{wildcard}\" (the wildcard for all nodes).\n"
                f"  In this exercise, use the node name \"{NODE_NAME}\" itself as the key.\n"
                f"  Expected key: {NODE_NAME} / actual keys: {list(root.keys())}",
            ))
        pytest.fail(localized(
            f"トップレベルに \"{NODE_NAME}\" というキーがありません。\n"
            f"  期待するキー: {NODE_NAME}\n"
            f"  実際のキー:   {list(root.keys())}\n"
            f"  {NODE_NAME}:\n    ros__parameters:\n      ... の3段構造になっていますか？",
            f"There is no \"{NODE_NAME}\" key at the top level.\n"
            f"  Expected key: {NODE_NAME}\n"
            f"  Actual keys:  {list(root.keys())}\n"
            f"  Is it a 3-level structure like {NODE_NAME}:\n    ros__parameters:\n      ... ?",
        ))

    node_body = root[NODE_NAME]
    if not isinstance(node_body, dict):
        pytest.fail(localized(
            f"\"{NODE_NAME}:\" の下は辞書である必要があります（ros__parameters を持つ階層）。\n"
            f"  実際の型: {type(node_body).__name__} / 実際の値: {node_body!r}",
            f"Under \"{NODE_NAME}:\" there must be a dictionary (the level that has ros__parameters).\n"
            f"  Actual type: {type(node_body).__name__} / actual value: {node_body!r}",
        ))

    if "ros__parameters" not in node_body:
        near_misses = [
            k for k in node_body
            if isinstance(k, str) and "parameters" in k.replace("_", "")
        ]
        hint = ""
        if near_misses:
            hint = localized(
                f"\n  似たキーが見つかりました: {near_misses}\n"
                "  \"ros__parameters\" はアンダースコアが2本です（ros_parameters ではありません）。",
                f"\n  Found similar keys: {near_misses}\n"
                "  \"ros__parameters\" has two underscores (not ros_parameters).",
            )
        pytest.fail(localized(
            f"\"{NODE_NAME}:\" の下に \"ros__parameters\" キーがありません。\n"
            f"  実際のキー: {list(node_body.keys())}{hint}\n"
            "  正しい3段構造:\n"
            f"    {NODE_NAME}:\n      ros__parameters:\n        my_parameter: ...\n",
            f"There is no \"ros__parameters\" key under \"{NODE_NAME}:\".\n"
            f"  Actual keys: {list(node_body.keys())}{hint}\n"
            "  Correct 3-level structure:\n"
            f"    {NODE_NAME}:\n      ros__parameters:\n        my_parameter: ...\n",
        ))

    params = node_body["ros__parameters"]
    if not isinstance(params, dict):
        pytest.fail(localized(
            "\"ros__parameters:\" の下は辞書である必要があります（パラメータ名: 値 の階層）。\n"
            f"  実際の型: {type(params).__name__} / 実際の値: {params!r}",
            "Under \"ros__parameters:\" there must be a dictionary (the level of parameter name: value).\n"
            f"  Actual type: {type(params).__name__} / actual value: {params!r}",
        ))
    return params


# ---------------------------------------------------------------------------
# 1. YAML として読めるか
# ---------------------------------------------------------------------------


def test_params_yaml_is_valid_yaml():
    root = _load_yaml()
    assert root is None or isinstance(root, (dict, list)), localized(
        f"{YAML_PATH} の読み込み結果が想定外の型です: {type(root).__name__}",
        f"The result of loading {YAML_PATH} has an unexpected type: {type(root).__name__}",
    )


# ---------------------------------------------------------------------------
# 2. 3段構造になっているか
# ---------------------------------------------------------------------------


def test_has_three_level_structure():
    root = _load_yaml()
    params = _get_ros_parameters(root)
    assert isinstance(params, dict), localized(
        f"ros__parameters の中身が辞書ではありません: {params!r}",
        f"The content of ros__parameters is not a dictionary: {params!r}",
    )


# ---------------------------------------------------------------------------
# 3. 4つのパラメータが正しい型と値か
# ---------------------------------------------------------------------------


def test_four_parameters_have_correct_types_and_values():
    root = _load_yaml()
    params = _get_ros_parameters(root)

    missing = [k for k in EXPECTED if k not in params]
    if missing:
        pytest.fail(localized(
            f"ros__parameters に次のキーが足りません: {missing}\n"
            f"  現在のキー: {list(params.keys())}",
            f"ros__parameters is missing these keys: {missing}\n"
            f"  Current keys: {list(params.keys())}",
        ))

    value = params["my_parameter"]
    assert isinstance(value, str), localized(
        "my_parameter は文字列である必要があります。\n"
        f"  期待する型: str / 実際の型: {type(value).__name__} / 実際の値: {value!r}\n"
        "  YAML で bonjour をクォートし忘れていませんか？",
        "my_parameter must be a string.\n"
        f"  Expected type: str / actual type: {type(value).__name__} / actual value: {value!r}\n"
        "  Did you forget to quote bonjour in YAML?",
    )
    assert value == "bonjour", localized(
        f"my_parameter の値が違います。期待値: \"bonjour\" / 実際の値: {value!r}",
        f"The value of my_parameter is wrong. Expected: \"bonjour\" / actual: {value!r}",
    )

    value = params["an_int_param"]
    assert isinstance(value, int) and not isinstance(value, bool), localized(
        "an_int_param は整数である必要があります。\n"
        f"  期待する型: int / 実際の型: {type(value).__name__} / 実際の値: {value!r}\n"
        "  クォートで囲って文字列 \"7\" にしていませんか？",
        "an_int_param must be an integer.\n"
        f"  Expected type: int / actual type: {type(value).__name__} / actual value: {value!r}\n"
        "  Did you put quotes around it and make it the string \"7\"?",
    )
    assert value == 7, localized(
        f"an_int_param の値が違います。期待値: 7 / 実際の値: {value!r}",
        f"The value of an_int_param is wrong. Expected: 7 / actual: {value!r}",
    )

    value = params["a_double_param"]
    assert isinstance(value, float), localized(
        "a_double_param は浮動小数である必要があります。\n"
        f"  期待する型: float / 実際の型: {type(value).__name__} / 実際の値: {value!r}\n"
        "  YAML では 1.5 のように小数点を書くと float になります（7 と書くと int になります）。",
        "a_double_param must be a floating-point number.\n"
        f"  Expected type: float / actual type: {type(value).__name__} / actual value: {value!r}\n"
        "  In YAML, a decimal point like 1.5 makes it a float (writing 7 makes it an int).",
    )
    assert value == 1.5, localized(
        f"a_double_param の値が違います。期待値: 1.5 / 実際の値: {value!r}",
        f"The value of a_double_param is wrong. Expected: 1.5 / actual: {value!r}",
    )

    value = params["a_string_list"]
    assert isinstance(value, list), localized(
        "a_string_list はリストである必要があります。\n"
        f"  期待する型: list / 実際の型: {type(value).__name__} / 実際の値: {value!r}",
        "a_string_list must be a list.\n"
        f"  Expected type: list / actual type: {type(value).__name__} / actual value: {value!r}",
    )
    assert all(isinstance(v, str) for v in value), localized(
        f"a_string_list の要素はすべて文字列である必要があります。実際の値: {value!r}",
        f"All elements of a_string_list must be strings. Actual value: {value!r}",
    )
    assert value == ["alpha", "beta"], localized(
        f"a_string_list の値が違います。期待値: ['alpha', 'beta'] / 実際の値: {value!r}",
        f"The value of a_string_list is wrong. Expected: ['alpha', 'beta'] / actual: {value!r}",
    )


# ---------------------------------------------------------------------------
# 4. end-to-end: 実際に param_echo を起動して確認する
# ---------------------------------------------------------------------------


def _candidate_param_echo_paths():
    """param_echo 実行ファイルの候補パスを (説明, パス) のリストで返す。

    ./drill run はリポジトリ直下の install/ を使う。手元での動作確認や CI では
    別の install ディレクトリを使いたいことがあるので、環境変数での上書きと
    ament_index 経由の解決も候補に加える（source install/setup.bash 済みなら効く）。
    """
    candidates = []

    env_path = os.environ.get("DRILL_PARAM_ECHO_PATH")
    if env_path:
        candidates.append((localized("環境変数 DRILL_PARAM_ECHO_PATH", "environment variable DRILL_PARAM_ECHO_PATH"), Path(env_path)))

    env_base = os.environ.get("DRILL_INSTALL_BASE")
    if env_base:
        candidates.append((
            localized("環境変数 DRILL_INSTALL_BASE", "environment variable DRILL_INSTALL_BASE"),
            Path(env_base) / PACKAGE_NAME / "lib" / PACKAGE_NAME / "param_echo",
        ))

    try:
        from ament_index_python.packages import get_package_prefix
        prefix = get_package_prefix(PACKAGE_NAME)
        candidates.append((
            localized(
                "ament_index（install/setup.bash を source 済みの場合に見つかる）",
                "ament_index (found when install/setup.bash has been sourced)",
            ),
            Path(prefix) / "lib" / PACKAGE_NAME / "param_echo",
        ))
    except Exception:
        pass

    candidates.append((
        localized(
            "リポジトリ直下の install/（./drill run が使う場所）",
            "install/ at the repository root (where ./drill run looks)",
        ),
        REPO_ROOT / "install" / PACKAGE_NAME / "lib" / PACKAGE_NAME / "param_echo",
    ))

    return candidates


def _find_param_echo():
    candidates = _candidate_param_echo_paths()
    for _label, path in candidates:
        if path.exists():
            return path
    checked = "\n".join(f"  - [{label}] {path}" for label, path in candidates)
    pytest.fail(localized(
        "param_echo の実行ファイルが見つかりません。先に colcon build してください。\n"
        f"  例: colcon build --packages-select {PACKAGE_NAME}\n"
        "  探した場所:\n" + checked,
        "The param_echo executable was not found. Run colcon build first.\n"
        f"  Example: colcon build --packages-select {PACKAGE_NAME}\n"
        "  Places searched:\n" + checked,
    ))


def test_param_echo_logs_correct_values_end_to_end():
    binary = _find_param_echo()

    cmd = [str(binary), "--ros-args", "--params-file", str(YAML_PATH)]
    try:
        proc = subprocess.run(
            cmd, capture_output=True, text=True, timeout=5,
        )
    except subprocess.TimeoutExpired:
        pytest.fail(localized(
            "param_echo が5秒たっても終了しませんでした。\n"
            "  rclcpp::init -> ノード生成 -> ログ出力 -> rclcpp::shutdown -> return 0 の"
            "順で、spin せずに終了していますか？",
            "param_echo did not exit after 5 seconds.\n"
            "  Does it exit without spinning, in this order: rclcpp::init -> create node -> "
            "log output -> rclcpp::shutdown -> return 0?",
        ))

    output = proc.stdout + proc.stderr
    expected_lines = {
        "my_parameter": "my_parameter=bonjour",
        "an_int_param": "an_int_param=7",
        "a_double_param": "a_double_param=1.5",
        "a_string_list": "a_string_list=[alpha,beta]",
    }
    missing = [line for line in expected_lines.values() if line not in output]
    if missing:
        expected_text = "\n".join(f"    {line}" for line in expected_lines.values())
        missing_text = "\n".join(f"    {line}" for line in missing)
        pytest.fail(localized(
            "param_echo の出力に、期待する行がありませんでした。\n"
            f"  期待する行:\n{expected_text}\n"
            f"  見つからなかった行:\n{missing_text}\n\n"
            f"  実際の標準出力/標準エラー全体:\n{output}\n\n"
            f"  {YAML_PATH} の中身（4つのパラメータの型と値）を確認してください。",
            "The output of param_echo did not contain the expected lines.\n"
            f"  Expected lines:\n{expected_text}\n"
            f"  Lines not found:\n{missing_text}\n\n"
            f"  Full stdout/stderr:\n{output}\n\n"
            f"  Check the content of {YAML_PATH} (the types and values of the 4 parameters).",
        ))


# ---------------------------------------------------------------------------
# 5. launch/param_demo.launch.py が param_echo を正しく起動しているか
# ---------------------------------------------------------------------------


def _load_launch_module():
    if not LAUNCH_PATH.exists():
        pytest.fail(localized(
            f"{LAUNCH_PATH} が見つかりません。ファイルを作成してください。",
            f"{LAUNCH_PATH} was not found. Please create the file.",
        ))
    spec = importlib.util.spec_from_file_location("drill_param_demo_launch", LAUNCH_PATH)
    module = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(module)
    except Exception as e:  # noqa: BLE001 — 受講者コードの例外を分かりやすく変換する
        pytest.fail(localized(
            f"{LAUNCH_PATH} の読み込み中にエラーが発生しました: {e!r}\n"
            "  get_package_share_directory('drill_08_params_yaml') が失敗する場合は、"
            "先に colcon build して install/setup.bash を source してください。",
            f"An error occurred while loading {LAUNCH_PATH}: {e!r}\n"
            "  If get_package_share_directory('drill_08_params_yaml') fails, "
            "run colcon build first and source install/setup.bash.",
        ))
    return module


def test_launch_file_starts_param_echo_with_params_yaml():
    from launch import LaunchContext, LaunchDescription
    from launch.utilities import perform_substitutions
    from launch_ros.actions import Node as LaunchNode
    from launch_ros.parameter_descriptions import ParameterFile

    module = _load_launch_module()
    assert hasattr(module, "generate_launch_description"), localized(
        f"{LAUNCH_PATH} に generate_launch_description() 関数がありません。",
        f"{LAUNCH_PATH} has no generate_launch_description() function.",
    )

    ld = module.generate_launch_description()
    assert isinstance(ld, LaunchDescription), localized(
        "generate_launch_description() は LaunchDescription を返す必要があります。\n"
        f"  実際の戻り値の型: {type(ld).__name__}",
        "generate_launch_description() must return a LaunchDescription.\n"
        f"  Actual return type: {type(ld).__name__}",
    )

    nodes = [e for e in ld.entities if isinstance(e, LaunchNode)]
    assert len(nodes) == 1, localized(
        f"LaunchDescription の中に Node が1つある必要がありますが、{len(nodes)}個見つかりました。\n"
        "  Node(package='drill_08_params_yaml', executable='param_echo', ...) を"
        " LaunchDescription([...]) に渡していますか？",
        f"LaunchDescription must contain exactly 1 Node, but {len(nodes)} were found.\n"
        "  Do you pass Node(package='drill_08_params_yaml', executable='param_echo', ...) "
        "to LaunchDescription([...])?",
    )
    node = nodes[0]

    assert node.node_package == PACKAGE_NAME, localized(
        f"Node の package が違います。期待値: '{PACKAGE_NAME}' / 実際の値: {node.node_package!r}",
        f"The package of the Node is wrong. Expected: '{PACKAGE_NAME}' / actual: {node.node_package!r}",
    )
    assert node.node_executable == "param_echo", localized(
        "Node の executable が違います。"
        f"期待値: 'param_echo' / 実際の値: {node.node_executable!r}",
        "The executable of the Node is wrong. "
        f"Expected: 'param_echo' / actual: {node.node_executable!r}",
    )

    raw_params = getattr(node, "_Node__parameters", None)
    assert raw_params, localized(
        "Node に parameters が渡されていません。\n"
        "  parameters=[params_file] を渡していますか？",
        "No parameters were passed to the Node.\n"
        "  Do you pass parameters=[params_file]?",
    )

    context = LaunchContext()
    resolved_paths = []
    for p in raw_params:
        if isinstance(p, ParameterFile):
            subs = p.param_file
            if isinstance(subs, (str, os.PathLike)):
                resolved_paths.append(str(subs))
            else:
                resolved_paths.append(perform_substitutions(context, subs))
    assert resolved_paths, localized(
        "Node の parameters に YAML ファイルへのパス（文字列）が見つかりませんでした。\n"
        f"  実際の parameters: {raw_params!r}\n"
        "  parameters=[params_file]（辞書ではなくファイルパス）を渡してください。",
        "No path (string) to a YAML file was found in the parameters of the Node.\n"
        f"  Actual parameters: {raw_params!r}\n"
        "  Pass parameters=[params_file] (a file path, not a dictionary).",
    )

    target = Path(resolved_paths[0])
    assert target.parts[-2:] == ("config", "params.yaml"), localized(
        "parameters に渡しているパスが config/params.yaml を指していません。\n"
        f"  実際のパス: {target}\n"
        "  os.path.join(get_package_share_directory('drill_08_params_yaml'), "
        "'config', 'params.yaml') で組み立てていますか？",
        "The path passed to parameters does not point to config/params.yaml.\n"
        f"  Actual path: {target}\n"
        "  Do you build it with os.path.join(get_package_share_directory('drill_08_params_yaml'), "
        "'config', 'params.yaml')?",
    )
    assert target.exists(), localized(
        f"parameters に渡しているパスが実在しません: {target}\n"
        "  先に colcon build してください"
        "（config/ が share/drill_08_params_yaml/ にインストールされている必要があります）。",
        f"The path passed to parameters does not exist: {target}\n"
        "  Run colcon build first "
        "(config/ must be installed in share/drill_08_params_yaml/).",
    )
