"""YOLO 学習用のデータセット規約。学習そのものは採点しません。"""


def yolo_line(class_id, x_min, y_min, x_max, y_max, image_w, image_h):
    if class_id < 0 or image_w <= 0 or image_h <= 0:
        raise ValueError("class id and image size must be positive")
    if x_min < 0 or y_min < 0 or x_max > image_w or y_max > image_h:
        raise ValueError("box is outside the image")
    if x_max <= x_min or y_max <= y_min:
        raise ValueError("box has no area")
    cx = ((x_min + x_max) / 2.0) / image_w
    cy = ((y_min + y_max) / 2.0) / image_h
    width = (x_max - x_min) / image_w
    height = (y_max - y_min) / image_h
    return f"{class_id} {cx:.6f} {cy:.6f} {width:.6f} {height:.6f}"


def split_records(names, val_every):
    if val_every <= 1:
        raise ValueError("val_every must be greater than 1")
    train = []
    val = []
    for index, name in enumerate(names):
        if (index + 1) % val_every == 0:
            val.append(name)
        else:
            train.append(name)
    return train, val


def export_argv(weights_path, opset):
    if not weights_path or opset < 12:
        raise ValueError("weights and opset")
    return [
        "yolo",
        "export",
        f"model={weights_path}",
        "format=onnx",
        f"opset={opset}",
        "simplify=True",
    ]


def robocon_class_names():
    return ["ball", "goal_marker", "opponent", "start_pole"]
