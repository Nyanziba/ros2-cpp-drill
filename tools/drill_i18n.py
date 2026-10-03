"""ROS 2 練習帳 テストの文言を日英で切り替えるヘルパ（pytest の課題用）。

このファイルは編集しません。DRILL_LANG=en のときだけ英語の文言を返します
（./drill の localized() と同じ規則）。
"""

import os


def is_english():
    """DRILL_LANG が "en" で始まるか（大文字小文字は区別しない）。"""
    return os.environ.get("DRILL_LANG", "").lower().startswith("en")


def localized(japanese, english):
    """失敗メッセージを言語に合わせて選ぶ。"""
    return english if is_english() else japanese
