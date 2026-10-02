#!/usr/bin/env python3
"""全課題について「未解答では落ち、解答例を当てれば通る」ことを確かめる。

使い方:
    python3 tools/verify_exercises.py                  全課題
    python3 tools/verify_exercises.py --track cppb     トラックで絞る
    python3 tools/verify_exercises.py --id cppb06 --id 01_publisher

トラックは課題 ID の接頭辞で決まる。
    c      c01_...    C 言語編
    cppb   cppb01_... C++ 入門編
    cpp    cpp01_...  C++ 編
    dp     dp01_...   デザインパターン編
    ros2   01_...     ROS 2 編（ID が数字で始まる）

どの課題も最後に必ず templates/<id>/ の内容で exercises/<id>/ を戻す。
終了コードは、期待に反した課題が 1 つでもあれば 1。
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
EXERCISES_DIRECTORY = REPOSITORY_ROOT / "exercises"
SOLUTIONS_DIRECTORY = REPOSITORY_ROOT / "solutions"
TEMPLATES_DIRECTORY = REPOSITORY_ROOT / "templates"
TRACKS = ("c", "cppb", "cpp", "dp", "ros2")
FAILURE_TAIL_LINE_COUNT = 8


@dataclass(frozen=True)
class Verdict:
    exercise_id: str
    unsolved_ok: bool
    solved_ok: bool
    note: str
    seconds: float

    @property
    def is_expected(self):
        return self.unsolved_ok and self.solved_ok


def track_of(exercise_id):
    prefix = re.match(r"[a-z]*", exercise_id).group()
    return prefix if prefix else "ros2"


def load_exercise_ids():
    config = json.loads((REPOSITORY_ROOT / "exercises.json").read_text(encoding="utf-8"))
    return [exercise["id"] for exercise in config["exercises"]]


def select_exercise_ids(all_ids, track, requested_ids):
    if requested_ids:
        unknown = [requested for requested in requested_ids if requested not in all_ids]
        if unknown:
            sys.exit(f"exercises.json にない ID です: {', '.join(unknown)}")
        return list(requested_ids)
    if track:
        return [exercise_id for exercise_id in all_ids if track_of(exercise_id) == track]
    return list(all_ids)


def overlay(source_directory, destination_directory):
    """source の中身を destination に上書きコピーする（destination の他のファイルは残す）。"""
    # copyfile は更新時刻を引き継がない。引き継ぐと、ビルド済みの成果物より
    # 古いファイルになって make が再コンパイルせず、前回の結果を拾ってしまう。
    shutil.copytree(source_directory, destination_directory,
                    copy_function=shutil.copyfile, dirs_exist_ok=True)


def run_drill(exercise_id):
    # DRILL_LANG が立っていると出力が英語になる。表示は日本語のままにしたい。
    environment = {key: value for key, value in os.environ.items() if key != "DRILL_LANG"}
    return subprocess.run(
        [sys.executable, str(REPOSITORY_ROOT / "drill"), "run", exercise_id],
        cwd=REPOSITORY_ROOT, env=environment,
        capture_output=True, text=True, errors="replace",
    )


def tail(result):
    lines = (result.stdout + result.stderr).strip().splitlines()
    return " / ".join(line.strip() for line in lines[-FAILURE_TAIL_LINE_COUNT:])


def verify_one(exercise_id):
    started_at = time.monotonic()
    exercise_directory = EXERCISES_DIRECTORY / exercise_id
    note = ""
    try:
        unsolved_result = run_drill(exercise_id)
        unsolved_ok = unsolved_result.returncode != 0
        if not unsolved_ok:
            note = "未解答なのに通った"

        overlay(SOLUTIONS_DIRECTORY / exercise_id, exercise_directory)
        solved_result = run_drill(exercise_id)
        solved_ok = solved_result.returncode == 0
        if not solved_ok:
            note = (note + " | " if note else "") + "解答例で落ちた: " + tail(solved_result)
    finally:
        # 例外や Ctrl-C でも未解答の状態に戻す。
        overlay(TEMPLATES_DIRECTORY / exercise_id, exercise_directory)
    return Verdict(exercise_id, unsolved_ok, solved_ok, note, time.monotonic() - started_at)


def mark(is_ok):
    return "OK" if is_ok else "NG"


def print_table(verdicts):
    print()
    print(f"{'課題':<28} {'未解答で失敗':<12} {'解答例で成功':<12} {'秒':>6}")
    for verdict in verdicts:
        print(f"{verdict.exercise_id:<28} {mark(verdict.unsolved_ok):<12} "
              f"{mark(verdict.solved_ok):<12} {verdict.seconds:>6.0f}")
    print()
    for verdict in verdicts:
        if not verdict.is_expected:
            print(f"期待に反した課題 {verdict.exercise_id}: {verdict.note}")


def parse_arguments():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--track", choices=TRACKS, help="このトラックの課題だけ確かめる")
    parser.add_argument("--id", action="append", dest="ids", default=[],
                        help="この課題だけ確かめる（複数指定可。完全な ID）")
    return parser.parse_args()


def main():
    arguments = parse_arguments()
    exercise_ids = select_exercise_ids(load_exercise_ids(), arguments.track, arguments.ids)
    verdicts = []
    for index, exercise_id in enumerate(exercise_ids, start=1):
        print(f"[{index}/{len(exercise_ids)}] {exercise_id}", flush=True)
        verdicts.append(verify_one(exercise_id))
        print(f"    -> {'期待どおり' if verdicts[-1].is_expected else '期待に反した'}", flush=True)
    print_table(verdicts)
    unexpected_count = sum(1 for verdict in verdicts if not verdict.is_expected)
    print(f"{len(verdicts)} 課題中、期待に反したのは {unexpected_count} 課題")
    return 1 if unexpected_count else 0


if __name__ == "__main__":
    sys.exit(main())
