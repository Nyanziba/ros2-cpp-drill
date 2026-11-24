#!/usr/bin/env python3
"""同梱の color_marker.onnx を作り直す。課題の採点モデルなので、重みは変えない。"""

from pathlib import Path

import numpy as np
import onnx
from onnx import TensorProto, helper, numpy_helper

WEIGHTS = np.array(
    [
        [-0.3, -0.3, -0.3],
        [-0.5, -0.5, 1.0],
        [1.0, -0.5, -0.5],
    ],
    dtype=np.float32,
)
BIAS = np.array([0.2, 0.0, 0.0], dtype=np.float32)


def classify(mean_bgr):
    logits = np.asarray(mean_bgr, dtype=np.float32) @ WEIGHTS.T + BIAS
    return int(np.argmax(logits))


def main():
    checks = {
        (0.0, 0.0, 0.0): 0,
        (0.0, 0.0, 1.0): 1,
        (1.0, 0.0, 0.0): 2,
        (0.0, 1.0, 0.0): 0,
        (0.0, 0.0, 0.5): 1,
    }
    for mean, expected in checks.items():
        got = classify(mean)
        if got != expected:
            raise SystemExit(f"{mean}: expected {expected}, got {got}")

    mean = helper.make_tensor_value_info("mean_bgr", TensorProto.FLOAT, [1, 3])
    logits = helper.make_tensor_value_info("logits", TensorProto.FLOAT, [1, 3])
    node = helper.make_node("Gemm", ["mean_bgr", "W", "B"], ["logits"], transB=1)
    graph = helper.make_graph(
        [node],
        "color_marker",
        [mean],
        [logits],
        [
            numpy_helper.from_array(WEIGHTS, name="W"),
            numpy_helper.from_array(BIAS, name="B"),
        ],
    )
    model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 13)])
    model.ir_version = 8
    onnx.checker.check_model(model)
    destination = Path(__file__).with_name("color_marker.onnx")
    onnx.save(model, destination)
    print(destination)


if __name__ == "__main__":
    main()
