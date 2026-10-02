#!/usr/bin/env python3
"""読み物の Compiler Explorer 短縮リンクが、同じページのコードと一致するか検査する。

各リンク https://godbolt.org/z/<id> について、
https://godbolt.org/api/shortlinkinfo/<id> の sessions[0].source が、
同じページのいずれかのコードブロック（末尾の空白を除く）と一致するかを調べる。
外部サービスに頼るので、普段の CI ではなく定期実行（週 1 回）で使う。
ずれたリンクがあれば一覧を出して終了コード 1。
"""

import json
import re
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
DOCUMENT_DIRECTORIES = ("docs", "docs-en")

SHORTLINK_PATTERN = re.compile(r"https://godbolt\.org/z/([A-Za-z0-9_-]+)")
CODE_BLOCK_PATTERN = re.compile(r"^[ \t]*```[^\n]*\n(.*?)^[ \t]*```[ \t]*$", re.DOTALL | re.MULTILINE)
API_URL_TEMPLATE = "https://godbolt.org/api/shortlinkinfo/{shortlink_id}"
USER_AGENT = "ros2-cpp-drill-docs-check (https://github.com/Nyanziba/ros2-cpp-drill)"

SECONDS_BETWEEN_REQUESTS = 2  # 相手に負荷をかけない
SECONDS_AFTER_RATE_LIMIT = 30
MAXIMUM_ATTEMPTS = 5


def fetch_session_source(shortlink_id: str) -> str:
    request = urllib.request.Request(
        API_URL_TEMPLATE.format(shortlink_id=shortlink_id),
        headers={"User-Agent": USER_AGENT, "Accept": "application/json"},
    )
    for attempt in range(1, MAXIMUM_ATTEMPTS + 1):
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                return json.load(response)["sessions"][0]["source"]
        except urllib.error.HTTPError as error:
            if error.code != 429 or attempt == MAXIMUM_ATTEMPTS:
                raise
            time.sleep(SECONDS_AFTER_RATE_LIMIT * attempt)
    raise AssertionError("到達しない")


def normalize(source: str) -> str:
    return source.rstrip()


def find_mismatched_links(page_path: Path) -> list[str]:
    text = page_path.read_text(encoding="utf-8")
    code_blocks = {normalize(block) for block in CODE_BLOCK_PATTERN.findall(text)}
    mismatches = []
    for shortlink_id in dict.fromkeys(SHORTLINK_PATTERN.findall(text)):
        relative = page_path.relative_to(REPOSITORY_ROOT)
        try:
            source = normalize(fetch_session_source(shortlink_id))
        except (urllib.error.URLError, KeyError, IndexError, json.JSONDecodeError) as error:
            mismatches.append(f"{relative}: https://godbolt.org/z/{shortlink_id} を取得できない（{error}）")
        else:
            if source not in code_blocks:
                mismatches.append(f"{relative}: https://godbolt.org/z/{shortlink_id} がページのコードと一致しない")
        time.sleep(SECONDS_BETWEEN_REQUESTS)
    return mismatches


def main() -> int:
    page_paths = sorted(
        path for directory in DOCUMENT_DIRECTORIES for path in (REPOSITORY_ROOT / directory).rglob("*.md")
    )
    checked_pages = [path for path in page_paths if SHORTLINK_PATTERN.search(path.read_text(encoding="utf-8"))]
    failures = []
    for page_path in checked_pages:
        page_failures = find_mismatched_links(page_path)
        failures.extend(page_failures)
        print(f"{page_path.relative_to(REPOSITORY_ROOT)}: {'NG' if page_failures else 'ok'}", flush=True)
    if failures:
        print(f"{len(failures)} 本のリンクがずれています。")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print(f"OK: {len(checked_pages)} ページのリンクがすべてコードと一致しました。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
