# このファイルは編集しません（採点用）。
import importlib.util
from pathlib import Path

import pytest

import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools"))
from drill_i18n import localized  # noqa: E402

SPEC = importlib.util.spec_from_file_location(
    "yolo_dataset", Path(__file__).resolve().parents[1] / "src" / "yolo_dataset.py"
)
yolo = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(yolo)


def test_yolo_line_normalizes_box():
    line = yolo.yolo_line(1, 0, 0, 100, 50, 200, 100)
    assert line == "1 0.250000 0.250000 0.500000 0.500000", localized(
        "cx, cy, w, h は画像サイズで割った値です。",
        "cx, cy, w, h are divided by the image size.",
    )


def test_yolo_line_allows_far_edge_and_rejects_outside():
    assert yolo.yolo_line(0, 0, 0, 20, 10, 20, 10).startswith("0 ")
    with pytest.raises(ValueError):
        yolo.yolo_line(0, 0, 0, 21, 10, 20, 10)
    with pytest.raises(ValueError):
        yolo.yolo_line(0, 5, 5, 5, 8, 20, 10)
    with pytest.raises(ValueError):
        yolo.yolo_line(-1, 0, 0, 10, 10, 20, 10)


def test_split_is_deterministic():
    train, val = yolo.split_records(["a", "b", "c", "d", "e", "f"], 5)
    assert train == ["a", "b", "c", "d", "f"]
    assert val == ["e"]
    with pytest.raises(ValueError):
        yolo.split_records(["a"], 1)


def test_export_argv_pins_onnx():
    assert yolo.export_argv("best.pt", 17) == [
        "yolo",
        "export",
        "model=best.pt",
        "format=onnx",
        "opset=17",
        "simplify=True",
    ]
    with pytest.raises(ValueError):
        yolo.export_argv("", 17)
    with pytest.raises(ValueError):
        yolo.export_argv("best.pt", 11)


def test_class_names_match_the_practice_field():
    assert yolo.robocon_class_names() == ["ball", "goal_marker", "opponent", "start_pole"]


@pytest.mark.skip(
    reason=localized(
        "GPU での YOLO 学習は CI に無いので採点しません。ラベルと export 引数だけを見ます。",
        "YOLO training on a GPU is not in CI, so it is not graded. Only labels and the export arguments are checked.",
    )
)
def test_gpu_training_is_not_run_in_ci():
    pass
