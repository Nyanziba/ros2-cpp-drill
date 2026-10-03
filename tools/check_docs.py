#!/usr/bin/env python3
"""読み物（docs/・docs-en/）と課題データ（exercises.json など）の整合を検査する。

標準ライブラリだけで動く。失敗があれば一覧を出して終了コード 1 で終わる。
使い方: python3 tools/check_docs.py
"""

import json
import re
import sys
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent

# README.md も README.en.md もまだ無い課題。既知の欠落なので許容する
# （実測すると c01 には日英どちらの README も無い。書かれたらこの集合から外すこと）。
KNOWN_MISSING_README = {"c01_split_compile"}

JAPANESE_PATTERN = re.compile(r"[぀-ゟ゠-ヿ一-鿿]")
# 英語版の読み物に残してよい日本語：日本語サイトへの URL を含む行
JAPANESE_ALLOWED_LINE_MARKER = "nyanziba.github.io"
# `[11章]` のように、日本語版のツール出力をそのまま引用したインラインコードも許す
INLINE_CODE_PATTERN = re.compile(r"`[^`]*`")

MAPPING_ROW_PATTERN = re.compile(r"^\|\s*`(docs/[^`]+\.md)`\s*\|\s*`(docs-en/[^`]+\.md)`\s*\|")
HEADING_PATTERN = re.compile(r"^#{1,6} ")
FENCE_PATTERN = re.compile(r"^\s*```")
TABLE_ROW_PATTERN = re.compile(r"^\|")


def read_lines(path: Path) -> list[str]:
    return path.read_text(encoding="utf-8").splitlines()


def list_markdown_files(directory: Path) -> set[str]:
    return {
        path.relative_to(REPOSITORY_ROOT).as_posix()
        for path in directory.rglob("*.md")
    }


def load_translation_pairs() -> list[tuple[str, str]]:
    pairs = []
    for line in read_lines(REPOSITORY_ROOT / "TRANSLATING.md"):
        match = MAPPING_ROW_PATTERN.match(line)
        if match:
            pairs.append((match.group(1), match.group(2)))
    return pairs


def check_translation_table(pairs: list[tuple[str, str]]) -> list[str]:
    failures = []
    japanese_listed = [japanese for japanese, _ in pairs]
    english_listed = [english for _, english in pairs]
    for listed, label in ((japanese_listed, "日本語"), (english_listed, "英語")):
        for duplicated in sorted({name for name in listed if listed.count(name) > 1}):
            failures.append(f"[対応表] {label}側に重複: {duplicated}")
    for japanese, english in pairs:
        for name in (japanese, english):
            if not (REPOSITORY_ROOT / name).is_file():
                failures.append(f"[対応表] ファイルが無い: {name}")
    for directory, listed in (("docs", japanese_listed), ("docs-en", english_listed)):
        for name in sorted(list_markdown_files(REPOSITORY_ROOT / directory) - set(listed)):
            failures.append(f"[対応表] 表に載っていない: {name}")
    return failures


def count_page_structure(path: Path) -> dict[str, int]:
    lines = read_lines(path)
    return {
        "見出し": sum(1 for line in lines if HEADING_PATTERN.match(line)),
        "コードフェンス": sum(1 for line in lines if FENCE_PATTERN.match(line)),
        "<details": sum(line.count("<details") for line in lines),
        "表の行": sum(1 for line in lines if TABLE_ROW_PATTERN.match(line)),
    }


def check_page_structure(pairs: list[tuple[str, str]]) -> list[str]:
    failures = []
    for japanese, english in pairs:
        japanese_path = REPOSITORY_ROOT / japanese
        english_path = REPOSITORY_ROOT / english
        if not (japanese_path.is_file() and english_path.is_file()):
            continue  # 実在しないことは対応表の検査が報告する
        japanese_counts = count_page_structure(japanese_path)
        english_counts = count_page_structure(english_path)
        for item, japanese_count in japanese_counts.items():
            english_count = english_counts[item]
            if japanese_count != english_count:
                failures.append(
                    f"[構造] {japanese} と {english}: {item} が違う"
                    f"（日本語 {japanese_count} / 英語 {english_count}）"
                )
    return failures


def check_no_japanese_in_english_docs() -> list[str]:
    failures = []
    for name in sorted(list_markdown_files(REPOSITORY_ROOT / "docs-en")):
        for line_number, line in enumerate(read_lines(REPOSITORY_ROOT / name), start=1):
            if JAPANESE_ALLOWED_LINE_MARKER in line:
                continue
            if JAPANESE_PATTERN.search(INLINE_CODE_PATTERN.sub("", line)):
                failures.append(f"[英語版の日本語] {name}:{line_number}: {line.strip()[:60]}")
    return failures


def contains_japanese(value) -> bool:
    """文字列・リスト・辞書の中に日本語が 1 つでもあるか。"""
    if isinstance(value, str):
        return bool(JAPANESE_PATTERN.search(value))
    if isinstance(value, list):
        return any(contains_japanese(item) for item in value)
    if isinstance(value, dict):
        return any(contains_japanese(item) for item in value.values())
    return False


def check_exercise_entry(exercise: dict) -> list[str]:
    failures = []
    exercise_id = exercise["id"]

    exercise_directory = REPOSITORY_ROOT / "exercises" / exercise_id
    for kind in ("exercises", "templates", "solutions"):
        if not (REPOSITORY_ROOT / kind / exercise_id).is_dir():
            failures.append(f"[課題] {kind}/{exercise_id}/ が無い")
    for source in exercise.get("sources", []):
        for kind in ("exercises", "templates", "solutions"):
            if not (REPOSITORY_ROOT / kind / exercise_id / source).is_file():
                failures.append(f"[課題] {kind}/{exercise_id}/{source} が無い")

    for key in ("lecture", "lecture_en"):
        for lecture in exercise.get(key, []):
            if not (REPOSITORY_ROOT / lecture["path"]).is_file():
                failures.append(f"[課題] {exercise_id}: {key} の path が無い: {lecture['path']}")

    for key, value in exercise.items():
        if key.endswith("_en") and contains_japanese(value):
            failures.append(f"[課題] {exercise_id}: {key} に日本語が残っている")
    if len(exercise.get("hints_en", [])) != len(exercise.get("hints", [])):
        failures.append(
            f"[課題] {exercise_id}: hints_en の数が hints と違う"
            f"（{len(exercise.get('hints', []))} / {len(exercise.get('hints_en', []))}）"
        )

    if exercise_id in KNOWN_MISSING_README:
        return failures
    for readme in ("README.md", "README.en.md"):
        if not (exercise_directory / readme).is_file():
            failures.append(f"[課題] exercises/{exercise_id}/{readme} が無い")
    return failures


def check_exercises_json() -> list[str]:
    data = json.loads((REPOSITORY_ROOT / "exercises.json").read_text(encoding="utf-8"))
    failures = []
    for exercise in data["exercises"]:
        failures.extend(check_exercise_entry(exercise))
    return failures


def check_templates_match_exercises() -> list[str]:
    failures = []
    for template_directory in sorted((REPOSITORY_ROOT / "templates").iterdir()):
        if not template_directory.is_dir():
            continue
        for template_file in sorted(template_directory.rglob("*")):
            if not template_file.is_file():
                continue
            relative = template_file.relative_to(template_directory)
            exercise_file = REPOSITORY_ROOT / "exercises" / template_directory.name / relative
            if not exercise_file.is_file():
                failures.append(f"[テンプレート] exercises 側に無い: {exercise_file.relative_to(REPOSITORY_ROOT)}")
            elif template_file.read_bytes() != exercise_file.read_bytes():
                failures.append(
                    f"[テンプレート] 内容が違う: templates/{template_directory.name}/{relative}"
                    " と exercises 側（未解答の状態でない）"
                )
    return failures


def main() -> int:
    pairs = load_translation_pairs()
    failures = [
        *check_translation_table(pairs),
        *check_page_structure(pairs),
        *check_no_japanese_in_english_docs(),
        *check_exercises_json(),
        *check_templates_match_exercises(),
    ]
    if failures:
        print(f"{len(failures)} 件の不整合があります。")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print(f"OK: 対応表 {len(pairs)} 組、課題の整合、テンプレートを検査しました。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
