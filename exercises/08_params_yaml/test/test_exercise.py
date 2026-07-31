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
from pathlib import Path

import pytest
import yaml

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
        pytest.fail(f"{YAML_PATH} が見つかりません。ファイルを作成してください。")
    text = YAML_PATH.read_text(encoding="utf-8")
    try:
        return yaml.safe_load(text)
    except yaml.YAMLError as e:
        pytest.fail(
            f"{YAML_PATH} の YAML の構文エラーです: {e}\n"
            "  インデントは半角スペースのみ（タブ禁止）、"
            "\":\" の後には半角スペースが必要です。"
        )


def _get_ros_parameters(root, *, source=YAML_PATH):
    """root から <ノード名>.ros__parameters の dict を取り出す。

    3段構造になっていない場合は、原因を具体的に指摘して pytest.fail する。
    """
    if root is None:
        pytest.fail(
            f"{source} の中身が空です（コメントだけ、または \"I AM NOT DONE\" のままかもしれません）。\n"
            "  次の3段構造で書いてください:\n"
            f"    {NODE_NAME}:\n      ros__parameters:\n        my_parameter: ...\n"
        )
    if not isinstance(root, dict):
        pytest.fail(
            f"{source} のトップレベルは辞書（マッピング）である必要があります。"
            f"実際の型: {type(root).__name__} / 実際の値: {root!r}"
        )

    if NODE_NAME not in root:
        wildcard = "/**"
        if wildcard in root:
            pytest.fail(
                f"トップレベルのキーが \"{wildcard}\"（全ノード共通ワイルドカード）になっています。\n"
                f"  この課題ではノード名そのもの \"{NODE_NAME}\" をキーにしてください。\n"
                f"  期待するキー: {NODE_NAME} / 実際のキー: {list(root.keys())}"
            )
        pytest.fail(
            f"トップレベルに \"{NODE_NAME}\" というキーがありません。\n"
            f"  期待するキー: {NODE_NAME}\n"
            f"  実際のキー:   {list(root.keys())}\n"
            f"  {NODE_NAME}:\n    ros__parameters:\n      ... の3段構造になっていますか？"
        )

    node_body = root[NODE_NAME]
    if not isinstance(node_body, dict):
        pytest.fail(
            f"\"{NODE_NAME}:\" の下は辞書である必要があります（ros__parameters を持つ階層）。\n"
            f"  実際の型: {type(node_body).__name__} / 実際の値: {node_body!r}"
        )

    if "ros__parameters" not in node_body:
        near_misses = [
            k for k in node_body
            if isinstance(k, str) and "parameters" in k.replace("_", "")
        ]
        hint = ""
        if near_misses:
            hint = (
                f"\n  似たキーが見つかりました: {near_misses}\n"
                "  \"ros__parameters\" はアンダースコアが2本です（ros_parameters ではありません）。"
            )
        pytest.fail(
            f"\"{NODE_NAME}:\" の下に \"ros__parameters\" キーがありません。\n"
            f"  実際のキー: {list(node_body.keys())}{hint}\n"
            "  正しい3段構造:\n"
            f"    {NODE_NAME}:\n      ros__parameters:\n        my_parameter: ...\n"
        )

    params = node_body["ros__parameters"]
    if not isinstance(params, dict):
        pytest.fail(
            "\"ros__parameters:\" の下は辞書である必要があります（パラメータ名: 値 の階層）。\n"
            f"  実際の型: {type(params).__name__} / 実際の値: {params!r}"
        )
    return params


# ---------------------------------------------------------------------------
# 1. YAML として読めるか
# ---------------------------------------------------------------------------


def testパラメータYAMLが構文として読み込める():
    root = _load_yaml()
    assert root is None or isinstance(root, (dict, list)), (
        f"{YAML_PATH} の読み込み結果が想定外の型です: {type(root).__name__}"
    )


# ---------------------------------------------------------------------------
# 2. 3段構造になっているか
# ---------------------------------------------------------------------------


def test_3段構造になっている_ノード名からros__parametersまで():
    root = _load_yaml()
    params = _get_ros_parameters(root)
    assert isinstance(params, dict), (
        f"ros__parameters の中身が辞書ではありません: {params!r}"
    )


# ---------------------------------------------------------------------------
# 3. 4つのパラメータが正しい型と値か
# ---------------------------------------------------------------------------


def test_4つのパラメータが正しい型と値になっている():
    root = _load_yaml()
    params = _get_ros_parameters(root)

    missing = [k for k in EXPECTED if k not in params]
    if missing:
        pytest.fail(
            f"ros__parameters に次のキーが足りません: {missing}\n"
            f"  現在のキー: {list(params.keys())}"
        )

    value = params["my_parameter"]
    assert isinstance(value, str), (
        "my_parameter は文字列である必要があります。\n"
        f"  期待する型: str / 実際の型: {type(value).__name__} / 実際の値: {value!r}\n"
        "  YAML で bonjour をクォートし忘れていませんか？"
    )
    assert value == "bonjour", (
        f"my_parameter の値が違います。期待値: \"bonjour\" / 実際の値: {value!r}"
    )

    value = params["an_int_param"]
    assert isinstance(value, int) and not isinstance(value, bool), (
        "an_int_param は整数である必要があります。\n"
        f"  期待する型: int / 実際の型: {type(value).__name__} / 実際の値: {value!r}\n"
        "  クォートで囲って文字列 \"7\" にしていませんか？"
    )
    assert value == 7, f"an_int_param の値が違います。期待値: 7 / 実際の値: {value!r}"

    value = params["a_double_param"]
    assert isinstance(value, float), (
        "a_double_param は浮動小数である必要があります。\n"
        f"  期待する型: float / 実際の型: {type(value).__name__} / 実際の値: {value!r}\n"
        "  YAML では 1.5 のように小数点を書くと float になります（7 と書くと int になります）。"
    )
    assert value == 1.5, f"a_double_param の値が違います。期待値: 1.5 / 実際の値: {value!r}"

    value = params["a_string_list"]
    assert isinstance(value, list), (
        "a_string_list はリストである必要があります。\n"
        f"  期待する型: list / 実際の型: {type(value).__name__} / 実際の値: {value!r}"
    )
    assert all(isinstance(v, str) for v in value), (
        f"a_string_list の要素はすべて文字列である必要があります。実際の値: {value!r}"
    )
    assert value == ["alpha", "beta"], (
        f"a_string_list の値が違います。期待値: ['alpha', 'beta'] / 実際の値: {value!r}"
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
        candidates.append(("環境変数 DRILL_PARAM_ECHO_PATH", Path(env_path)))

    env_base = os.environ.get("DRILL_INSTALL_BASE")
    if env_base:
        candidates.append((
            "環境変数 DRILL_INSTALL_BASE",
            Path(env_base) / PACKAGE_NAME / "lib" / PACKAGE_NAME / "param_echo",
        ))

    try:
        from ament_index_python.packages import get_package_prefix
        prefix = get_package_prefix(PACKAGE_NAME)
        candidates.append((
            "ament_index（install/setup.bash を source 済みの場合に見つかる）",
            Path(prefix) / "lib" / PACKAGE_NAME / "param_echo",
        ))
    except Exception:
        pass

    candidates.append((
        "リポジトリ直下の install/（./drill run が使う場所）",
        REPO_ROOT / "install" / PACKAGE_NAME / "lib" / PACKAGE_NAME / "param_echo",
    ))

    return candidates


def _find_param_echo():
    candidates = _candidate_param_echo_paths()
    for _label, path in candidates:
        if path.exists():
            return path
    checked = "\n".join(f"  - [{label}] {path}" for label, path in candidates)
    pytest.fail(
        "param_echo の実行ファイルが見つかりません。先に colcon build してください。\n"
        f"  例: colcon build --packages-select {PACKAGE_NAME}\n"
        "  探した場所:\n" + checked
    )


def testEndToEndでparam_echoが正しい値をログに出す():
    binary = _find_param_echo()

    cmd = [str(binary), "--ros-args", "--params-file", str(YAML_PATH)]
    try:
        proc = subprocess.run(
            cmd, capture_output=True, text=True, timeout=5,
        )
    except subprocess.TimeoutExpired:
        pytest.fail(
            "param_echo が5秒たっても終了しませんでした。\n"
            "  rclcpp::init -> ノード生成 -> ログ出力 -> rclcpp::shutdown -> return 0 の"
            "順で、spin せずに終了していますか？"
        )

    output = proc.stdout + proc.stderr
    expected_lines = {
        "my_parameter": "my_parameter=bonjour",
        "an_int_param": "an_int_param=7",
        "a_double_param": "a_double_param=1.5",
        "a_string_list": "a_string_list=[alpha,beta]",
    }
    missing = [line for line in expected_lines.values() if line not in output]
    if missing:
        pytest.fail(
            "param_echo の出力に、期待する行がありませんでした。\n"
            "  期待する行:\n"
            + "\n".join(f"    {line}" for line in expected_lines.values())
            + "\n  見つからなかった行:\n"
            + "\n".join(f"    {line}" for line in missing)
            + f"\n\n  実際の標準出力/標準エラー全体:\n{output}\n\n"
            f"  {YAML_PATH} の中身（4つのパラメータの型と値）を確認してください。"
        )


# ---------------------------------------------------------------------------
# 5. launch/param_demo.launch.py が param_echo を正しく起動しているか
# ---------------------------------------------------------------------------


def _load_launch_module():
    if not LAUNCH_PATH.exists():
        pytest.fail(f"{LAUNCH_PATH} が見つかりません。ファイルを作成してください。")
    spec = importlib.util.spec_from_file_location("drill_param_demo_launch", LAUNCH_PATH)
    module = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(module)
    except Exception as e:  # noqa: BLE001 — 受講者コードの例外を分かりやすく変換する
        pytest.fail(
            f"{LAUNCH_PATH} の読み込み中にエラーが発生しました: {e!r}\n"
            "  get_package_share_directory('drill_08_params_yaml') が失敗する場合は、"
            "先に colcon build して install/setup.bash を source してください。"
        )
    return module


def test_launchファイルがparam_echoをparams_yaml付きで起動する():
    from launch import LaunchContext, LaunchDescription
    from launch.utilities import perform_substitutions
    from launch_ros.actions import Node as LaunchNode
    from launch_ros.parameter_descriptions import ParameterFile

    module = _load_launch_module()
    assert hasattr(module, "generate_launch_description"), (
        f"{LAUNCH_PATH} に generate_launch_description() 関数がありません。"
    )

    ld = module.generate_launch_description()
    assert isinstance(ld, LaunchDescription), (
        "generate_launch_description() は LaunchDescription を返す必要があります。\n"
        f"  実際の戻り値の型: {type(ld).__name__}"
    )

    nodes = [e for e in ld.entities if isinstance(e, LaunchNode)]
    assert len(nodes) == 1, (
        f"LaunchDescription の中に Node が1つある必要がありますが、{len(nodes)}個見つかりました。\n"
        "  Node(package='drill_08_params_yaml', executable='param_echo', ...) を"
        " LaunchDescription([...]) に渡していますか？"
    )
    node = nodes[0]

    assert node.node_package == PACKAGE_NAME, (
        f"Node の package が違います。期待値: '{PACKAGE_NAME}' / 実際の値: {node.node_package!r}"
    )
    assert node.node_executable == "param_echo", (
        "Node の executable が違います。"
        f"期待値: 'param_echo' / 実際の値: {node.node_executable!r}"
    )

    raw_params = getattr(node, "_Node__parameters", None)
    assert raw_params, (
        "Node に parameters が渡されていません。\n"
        "  parameters=[params_file] を渡していますか？"
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
    assert resolved_paths, (
        "Node の parameters に YAML ファイルへのパス（文字列）が見つかりませんでした。\n"
        f"  実際の parameters: {raw_params!r}\n"
        "  parameters=[params_file]（辞書ではなくファイルパス）を渡してください。"
    )

    target = Path(resolved_paths[0])
    assert target.parts[-2:] == ("config", "params.yaml"), (
        "parameters に渡しているパスが config/params.yaml を指していません。\n"
        f"  実際のパス: {target}\n"
        "  os.path.join(get_package_share_directory('drill_08_params_yaml'), "
        "'config', 'params.yaml') で組み立てていますか？"
    )
    assert target.exists(), (
        f"parameters に渡しているパスが実在しません: {target}\n"
        "  先に colcon build してください"
        "（config/ が share/drill_08_params_yaml/ にインストールされている必要があります）。"
    )
