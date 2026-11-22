# I AM NOT DONE
"""YOLO 学習用のデータセット規約。学習そのものは採点しません。"""


def yolo_line(class_id, x_min, y_min, x_max, y_max, image_w, image_h):
    """xyxy 画素を 'class cx cy w h'（正規化、小数6桁）にする。

    画像の外、幅か高さが 0 以下、class_id < 0 は ValueError。
    x_max == image_w と y_max == image_h は画像の内側なので許します。
    """
    raise NotImplementedError


def split_records(names, val_every):
    """0 始まりの添字 i について (i + 1) が val_every で割り切れるものを val にする。

    順番は保ちます。val_every <= 1 は ValueError。
    戻り値は (train, val)。
    """
    raise NotImplementedError


def export_argv(weights_path, opset):
    """ultralytics の export コマンドをリストで返す。

    ['yolo', 'export', f'model={weights_path}', 'format=onnx', f'opset={opset}', 'simplify=True']
    weights_path が空、または opset < 12 なら ValueError。
    """
    raise NotImplementedError


def robocon_class_names():
    """この練習フィールドのクラス名。順番が class id です。"""
    return []
