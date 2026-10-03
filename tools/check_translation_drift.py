#!/usr/bin/env python3
"""日本語版だけが変わった PR を見つけて警告する。

日本語版が正で英語版はその翻訳なので、英語版が古くなっても誰も気づかないのを防ぐ。
警告だけで落とさない（英語が難しい人は「英語版は未対応」と書けばよい）。

使い方: python3 tools/check_translation_drift.py <base の ref> <head の ref>
環境変数: PR_BODY（PR の本文）、GITHUB_STEP_SUMMARY（あれば Markdown の表を書く）
"""

import json
import os
import re
import subprocess
import sys
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
TRANSLATING_GUIDE_PATH = "TRANSLATING.md"
EXERCISES_JSON_PATH = "exercises.json"
UNTRANSLATED_MARKERS = ("英語版は未対応", "English not updated")
EXERCISE_JAPANESE_FIELDS = ("title", "hints", "level", "level_note", "chapter", "lecture", "docs")

FIXED_PAIRS = {
    "README.md": "README.en.md",
    "CONTRIBUTING.md": "CONTRIBUTING.en.md",
    "CODE_OF_CONDUCT.md": "CODE_OF_CONDUCT.en.md",
}
DOCS_TABLE_ROW_PATTERN = re.compile(r"^\| `(docs/[^`]+)` \| `(docs-en/[^`]+)` \|", re.MULTILINE)
EXERCISE_README_PATTERN = re.compile(r"^(exercises/[^/]+)/README\.md$")


def run_git(*arguments: str) -> str:
    # core.quotepath=false を付けないと日本語のファイル名が "\343..." と引用符つきで出る。
    completed = subprocess.run(
        ["git", "-c", "core.quotepath=false", *arguments],
        cwd=REPOSITORY_ROOT, capture_output=True, text=True, encoding="utf-8", check=True,
    )
    return completed.stdout


def read_file_at_ref(ref: str, path: str) -> str | None:
    try:
        return run_git("show", f"{ref}:{path}")
    except subprocess.CalledProcessError:
        return None


def load_docs_pairs(head_ref: str) -> dict[str, str]:
    guide_text = read_file_at_ref(head_ref, TRANSLATING_GUIDE_PATH) or ""
    return dict(DOCS_TABLE_ROW_PATTERN.findall(guide_text))


def find_english_counterpart(japanese_path: str, docs_pairs: dict[str, str]) -> str | None:
    if japanese_path in FIXED_PAIRS:
        return FIXED_PAIRS[japanese_path]
    if japanese_path in docs_pairs:
        return docs_pairs[japanese_path]
    exercise_match = EXERCISE_README_PATTERN.match(japanese_path)
    if exercise_match:
        return f"{exercise_match.group(1)}/README.en.md"
    return None


def load_exercises_by_id(ref: str) -> dict[str, dict]:
    text = read_file_at_ref(ref, EXERCISES_JSON_PATH)
    if text is None:
        return {}
    return {exercise["id"]: exercise for exercise in json.loads(text)["exercises"]}


def find_drifted_exercise_fields(base_ref: str, head_ref: str) -> list[tuple[str, str]]:
    """(課題 id, 説明) の一覧。日本語の項目だけが変わった課題を返す。"""
    base_exercises = load_exercises_by_id(base_ref)
    drifted = []
    for exercise_id, head_exercise in load_exercises_by_id(head_ref).items():
        base_exercise = base_exercises.get(exercise_id)
        if base_exercise is None:
            if not any(f"{field}_en" in head_exercise for field in EXERCISE_JAPANESE_FIELDS):
                drifted.append((exercise_id, "新しい課題に英語の項目（*_en）がありません"))
            continue
        stale_fields = [
            field for field in EXERCISE_JAPANESE_FIELDS
            if head_exercise.get(field) != base_exercise.get(field)
            and head_exercise.get(f"{field}_en") == base_exercise.get(f"{field}_en")
        ]
        if stale_fields:
            drifted.append((exercise_id, "日本語だけ変わった項目: " + ", ".join(stale_fields)))
    return drifted


def find_drifted_files(changed_paths: set[str], docs_pairs: dict[str, str]) -> list[tuple[str, str]]:
    """(日本語版のパス, 英語版のパス) の一覧。"""
    drifted = []
    for japanese_path in sorted(changed_paths):
        english_path = find_english_counterpart(japanese_path, docs_pairs)
        if english_path is not None and english_path not in changed_paths:
            drifted.append((japanese_path, english_path))
    return drifted


def has_untranslated_marker(pull_request_body: str) -> bool:
    return any(marker in pull_request_body for marker in UNTRANSLATED_MARKERS)


def build_summary(rows: list[tuple[str, str, str]], needs_declaration: bool) -> str:
    lines = []
    if needs_declaration:
        lines += ["**PR の本文に「英語版は未対応」と書くか、英語版も直してください。**", ""]
    lines += ["| 日本語版（変更あり） | 英語版（変更なし） |", "| --- | --- |"]
    lines += [f"| `{japanese}` | {english} |" for japanese, english, _ in rows]
    return "\n".join(lines) + "\n"


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    base_ref, head_ref = sys.argv[1], sys.argv[2]
    changed_paths = set(run_git("diff", "--name-only", f"{base_ref}...{head_ref}").splitlines())

    rows = [
        (japanese, f"`{english}`", japanese)
        for japanese, english in find_drifted_files(changed_paths, load_docs_pairs(head_ref))
    ]
    rows += [
        (f"{EXERCISES_JSON_PATH}（{exercise_id}）", detail, EXERCISES_JSON_PATH)
        for exercise_id, detail in find_drifted_exercise_fields(base_ref, head_ref)
    ] if EXERCISES_JSON_PATH in changed_paths else []

    if not rows:
        print("日本語版だけが変わったファイルはありません。")
        return 0

    needs_declaration = not has_untranslated_marker(os.environ.get("PR_BODY", ""))
    for japanese, english, annotated_file in rows:
        print(f"::warning file={annotated_file}::英語版が未更新です: {japanese} → {english}")
    summary = build_summary(rows, needs_declaration)
    print(summary)

    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_path:
        with open(summary_path, "a", encoding="utf-8") as summary_file:
            summary_file.write(summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
