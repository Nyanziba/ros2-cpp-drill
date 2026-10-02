# このファイルは編集しません（採点用）。
"""課題09 の採点用テスト。

talker_listener.launch.py / .launch.xml / .launch.yaml の 3 つの launch ファイルを
実際に読み込み（Python は import して generate_launch_description() を呼ぶ、
XML / YAML は launch.frontend.Parser で読む）、含まれる Node の

  package / executable / namespace / name / remap

を正規化して取り出し、3 つの書式がすべて同じ内容になっているかを確認します。

ノードの実行ファイルは起動しません（起動すると ros2 run が実際のプロセスを
立ち上げてしまうため）。Node._perform_substitutions() で置換だけを行い、
できあがる文字列を比較します。
"""
import functools
import importlib.util
import pathlib

import sys

import pytest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[3] / "tools"))
from drill_i18n import localized  # noqa: E402

from launch import LaunchContext
from launch import LaunchDescription
from launch.frontend import Parser
from launch.utilities import normalize_to_list_of_substitutions
from launch.utilities import perform_substitutions
from launch_ros.actions import Node

LAUNCH_DIR = pathlib.Path(__file__).resolve().parent.parent / "launch"
PY_FILE = LAUNCH_DIR / "talker_listener.launch.py"
XML_FILE = LAUNCH_DIR / "talker_listener.launch.xml"
YAML_FILE = LAUNCH_DIR / "talker_listener.launch.yaml"

# メッセージに埋め込む、launch ファイルの呼び名。
PYTHON_LABEL = localized("Python版", "Python launch file")
XML_LABEL = localized("XML版", "XML launch file")
YAML_LABEL = localized("YAML版", "YAML launch file")

EXPECTED_TALKER = {
    "package": "drill_01_publisher",
    "executable": "talker",
    "namespace": "/demo",
    "full_name": "/demo/talker",
    "remaps": [("topic", "chatter")],
}
EXPECTED_LISTENER = {
    "package": "drill_02_subscriber",
    "executable": "listener",
    "namespace": "/demo",
    "full_name": "/demo/listener",
    "remaps": [("topic", "chatter")],
}


def _canonicalize(ld: LaunchDescription) -> list:
    """LaunchDescription から Node だけを取り出し、比較しやすい辞書のリストにする。

    launch_ros.actions.Node は package / executable / name / namespace を
    Substitution（文字列そのものとは限らない）として保持しているので、
    LaunchContext を使って実際の文字列に変換する。
    Node.execute() は実プロセスを起動してしまうので、代わりに
    _perform_substitutions() だけを呼んで文字列展開のみ行う。
    """
    context = LaunchContext()
    nodes = []
    for entity in ld.entities:
        if not isinstance(entity, Node):
            continue
        entity._perform_substitutions(context)
        package = perform_substitutions(
            context, normalize_to_list_of_substitutions(entity.node_package))
        executable = perform_substitutions(
            context, normalize_to_list_of_substitutions(entity.node_executable))
        nodes.append({
            "package": package,
            "executable": executable,
            "namespace": entity.expanded_node_namespace,
            "full_name": entity.node_name,
            "remaps": sorted(entity.expanded_remapping_rules or []),
        })
    return nodes


@functools.lru_cache(maxsize=None)
def _load_python() -> list:
    """talker_listener.launch.py を読み込み、正規化した Node のリストを返す。

    例外で pytest 全体を落とさないよう、失敗はすべて pytest.fail() で
    分かりやすいメッセージに変換する。
    """
    if not PY_FILE.exists():
        pytest.fail(localized(
            f"{PY_FILE} が見つかりません。ファイルを作成してください。",
            f"{PY_FILE} was not found. Please create the file.",
        ))
    try:
        spec = importlib.util.spec_from_file_location(
            "drill_09_launch_talker_listener_py", PY_FILE)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    except Exception as exc:  # noqa: BLE001 - 受講者コードで何が起きるか分からないため
        pytest.fail(localized(
            f"{PY_FILE.name} の読み込み中に例外が発生しました: "
            f"{type(exc).__name__}: {exc}\n"
            "Python の構文エラーがないか確認してください。",
            f"An exception occurred while loading {PY_FILE.name}: "
            f"{type(exc).__name__}: {exc}\n"
            "Check for Python syntax errors.",
        ))
    if not hasattr(module, "generate_launch_description"):
        pytest.fail(localized(
            f"{PY_FILE.name} に generate_launch_description() 関数がありません。\n"
            "def generate_launch_description(): ... という関数を定義し、"
            "LaunchDescription を return してください。",
            f"{PY_FILE.name} has no generate_launch_description() function.\n"
            "Define def generate_launch_description(): ... and "
            "return a LaunchDescription.",
        ))
    try:
        ld = module.generate_launch_description()
    except Exception as exc:  # noqa: BLE001
        pytest.fail(localized(
            f"{PY_FILE.name} の generate_launch_description() 呼び出し中に"
            f"例外が発生しました: {type(exc).__name__}: {exc}",
            f"An exception occurred while calling generate_launch_description() "
            f"in {PY_FILE.name}: {type(exc).__name__}: {exc}",
        ))
    if not isinstance(ld, LaunchDescription):
        pytest.fail(localized(
            f"{PY_FILE.name} の generate_launch_description() が "
            f"LaunchDescription を返していません（実際の型: {type(ld).__name__}）。",
            f"generate_launch_description() in {PY_FILE.name} does not return "
            f"a LaunchDescription (actual type: {type(ld).__name__}).",
        ))
    return _canonicalize(ld)


def _load_frontend(path: pathlib.Path) -> list:
    """XML / YAML の launch ファイルを読み込み、正規化した Node のリストを返す。"""
    kind = "XML" if path.suffix == ".xml" else "YAML"
    if not path.exists():
        pytest.fail(localized(
            f"{path} が見つかりません。ファイルを作成してください。",
            f"{path} was not found. Please create the file.",
        ))
    try:
        root_entity, parser = Parser.load(str(path))
        ld = parser.parse_description(root_entity)
    except Exception as exc:  # noqa: BLE001 - 構文エラーを分かりやすいassert失敗に変換する
        pytest.fail(localized(
            f"{path.name} の{kind}の構文エラーです: {type(exc).__name__}: {exc}\n"
            "タグ／インデントの対応や、必須の属性（pkg, exec など）の"
            "書き忘れがないか、README.md の骨格と見比べてください。",
            f"{kind} syntax error in {path.name}: {type(exc).__name__}: {exc}\n"
            "Compare with the skeleton in README.md: are tags or indentation "
            "mismatched, or are required attributes (pkg, exec, etc.) missing?",
        ))
    try:
        return _canonicalize(ld)
    except Exception as exc:  # noqa: BLE001
        pytest.fail(localized(
            f"{path.name} を読み込んだ後の解釈でエラーが発生しました: "
            f"{type(exc).__name__}: {exc}\n"
            "name / namespace / remap の値が正しく書けているか確認してください。",
            f"An error occurred while interpreting {path.name} after loading it: "
            f"{type(exc).__name__}: {exc}\n"
            "Check that the values of name / namespace / remap are written correctly.",
        ))


@functools.lru_cache(maxsize=None)
def _load_xml() -> list:
    return _load_frontend(XML_FILE)


@functools.lru_cache(maxsize=None)
def _load_yaml() -> list:
    return _load_frontend(YAML_FILE)


def _find(nodes, package, executable):
    for node in nodes:
        if node["package"] == package and node["executable"] == executable:
            return node
    return None


def _describe(nodes):
    return [(n["package"], n["executable"]) for n in nodes]


def _assert_talker_and_listener(nodes, label):
    """talker / listener の 2 ノードがそろっていることを確認し、両方を返す。"""
    names = _describe(nodes)
    assert len(nodes) == 2, localized(
        f"{label}のノード数が期待と違います。期待: 2 個、実際: {len(nodes)} 個 {names}\n"
        "talker 用と listener 用、2 つの Node（node）を追加しましたか？",
        f"The number of nodes in the {label} is wrong. Expected: 2, actual: {len(nodes)} {names}\n"
        "Did you add 2 Nodes (node), one for talker and one for listener?",
    )
    talker = _find(nodes, EXPECTED_TALKER["package"], EXPECTED_TALKER["executable"])
    assert talker is not None, localized(
        f"{label}に talker ノード（package={EXPECTED_TALKER['package']}, "
        f"executable={EXPECTED_TALKER['executable']}）が見つかりません。\n"
        f"実際のノード（package, executable）: {names}",
        f"The {label} has no talker node (package={EXPECTED_TALKER['package']}, "
        f"executable={EXPECTED_TALKER['executable']}).\n"
        f"Actual nodes (package, executable): {names}",
    )
    listener = _find(nodes, EXPECTED_LISTENER["package"], EXPECTED_LISTENER["executable"])
    assert listener is not None, localized(
        f"{label}に listener ノード（package={EXPECTED_LISTENER['package']}, "
        f"executable={EXPECTED_LISTENER['executable']}）が見つかりません。\n"
        f"実際のノード（package, executable）: {names}",
        f"The {label} has no listener node (package={EXPECTED_LISTENER['package']}, "
        f"executable={EXPECTED_LISTENER['executable']}).\n"
        f"Actual nodes (package, executable): {names}",
    )
    return talker, listener


def test_python_launch_starts_talker_and_listener():
    """Python 版が talker / listener の2ノードを、正しい package / executable で持っているか。"""
    nodes = _load_python()
    _assert_talker_and_listener(nodes, PYTHON_LABEL)


def test_python_launch_has_demo_namespace_and_chatter_remap():
    """Python 版の namespace が demo、remap が topic→chatter になっているか。"""
    nodes = _load_python()
    talker, listener = _assert_talker_and_listener(nodes, PYTHON_LABEL)
    for label, node, expected in (
        ("talker", talker, EXPECTED_TALKER),
        ("listener", listener, EXPECTED_LISTENER),
    ):
        assert node["namespace"] == expected["namespace"], localized(
            f"{PYTHON_LABEL}の {label} の namespace が違います。"
            f"期待: '{expected['namespace']}'、実際: '{node['namespace']}'\n"
            'Node(..., namespace="demo", ...) を渡しましたか？',
            f"The namespace of {label} in the {PYTHON_LABEL} is wrong. "
            f"Expected: '{expected['namespace']}', actual: '{node['namespace']}'\n"
            'Did you pass Node(..., namespace="demo", ...)?',
        )
        assert node["remaps"] == expected["remaps"], localized(
            f"{PYTHON_LABEL}の {label} の remap が違います。"
            f"期待: {expected['remaps']}、実際: {node['remaps']}\n"
            'Node(..., remappings=[("topic", "chatter")], ...) を渡しましたか？',
            f"The remap of {label} in the {PYTHON_LABEL} is wrong. "
            f"Expected: {expected['remaps']}, actual: {node['remaps']}\n"
            'Did you pass Node(..., remappings=[("topic", "chatter")], ...)?',
        )


def test_xml_launch_matches_python_structure():
    """XML 版を Python 版と比較する。書式は違っても構造は一致するはず。"""
    py_nodes = _load_python()
    xml_nodes = _load_xml()
    _assert_talker_and_listener(xml_nodes, XML_LABEL)
    assert xml_nodes == py_nodes, localized(
        "XML版とPython版の構造が一致しません。\n"
        f"  Python版: {py_nodes}\n"
        f"  XML版:    {xml_nodes}\n"
        "<node pkg=\"...\" exec=\"...\" name=\"...\" namespace=\"...\"> と "
        "<remap from=\"...\" to=\"...\"/> を、README.md の表のとおりに書けていますか？",
        "The structure of the XML and Python launch files does not match.\n"
        f"  Python: {py_nodes}\n"
        f"  XML:    {xml_nodes}\n"
        "Did you write <node pkg=\"...\" exec=\"...\" name=\"...\" namespace=\"...\"> and "
        "<remap from=\"...\" to=\"...\"/> as in the table in README.md?",
    )


def test_yaml_launch_matches_python_structure():
    """YAML 版を Python 版と比較する。書式は違っても構造は一致するはず。"""
    py_nodes = _load_python()
    yaml_nodes = _load_yaml()
    _assert_talker_and_listener(yaml_nodes, YAML_LABEL)
    assert yaml_nodes == py_nodes, localized(
        "YAML版とPython版の構造が一致しません。\n"
        f"  Python版: {py_nodes}\n"
        f"  YAML版:   {yaml_nodes}\n"
        "launch: / node: / remap: の階層を、README.md の骨格のとおりに書けていますか？",
        "The structure of the YAML and Python launch files does not match.\n"
        f"  Python: {py_nodes}\n"
        f"  YAML:   {yaml_nodes}\n"
        "Did you write the launch: / node: / remap: levels as in the skeleton in README.md?",
    )


def test_all_three_formats_are_equivalent():
    """Python / XML / YAML、3 つの書式を正規化した構造がすべて一致するか。"""
    py_nodes = _load_python()
    xml_nodes = _load_xml()
    yaml_nodes = _load_yaml()
    # 3つとも空、のような自明な一致で通ってしまわないよう、まず中身があることを確認する。
    _assert_talker_and_listener(py_nodes, PYTHON_LABEL)
    _assert_talker_and_listener(xml_nodes, XML_LABEL)
    _assert_talker_and_listener(yaml_nodes, YAML_LABEL)
    assert py_nodes == xml_nodes == yaml_nodes, localized(
        "3つの書式（Python / XML / YAML）の構造が一致しません。\n"
        f"  Python版: {py_nodes}\n"
        f"  XML版:    {xml_nodes}\n"
        f"  YAML版:   {yaml_nodes}\n"
        "同じ内容（package / executable / name / namespace / remap）を"
        "3つの書式で書けているか見比べてください。",
        "The structure of the 3 formats (Python / XML / YAML) does not match.\n"
        f"  Python: {py_nodes}\n"
        f"  XML:    {xml_nodes}\n"
        f"  YAML:   {yaml_nodes}\n"
        "Compare them: is the same content (package / executable / name / namespace / remap) "
        "written in all 3 formats?",
    )
