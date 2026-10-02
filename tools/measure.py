#!/usr/bin/env python3
"""読み物に載せた出力を、実際に動かして測り直す（または食い違いを検査する）。

読み物では、測った出力のコードブロックの直前に印を置きます。

    <!-- measure: files=basics03_null.cpp filter="grep 'error:'" -->
    ```
    error: invalid initialization of ...
    ```

印の項目（どれも省略できます）:
  files   使うファイル（カンマ区切り）。それぞれ、印より前にある「1 行目が
          `// ファイル名` などのコメント」のコードブロックのうち、いちばん近いもの。
          印より前に無ければ、印より後ろでいちばん近いもの（出力を先に見せる書き方のため）。
          省略すると、印より前でいちばん近い、ファイル名つきのコードブロック 1 つ。
  cmd     実行するコマンド。省略すると、印より前でいちばん近い bash のコードブロック。
  env     gcc（既定。Docker の Ubuntu / g++）、clang（macOS の clang。手元が macOS のときだけ）、
          ros（Docker で ROS 2 を source し、~/ros2_ws をワークスペースにして動かす）、
          static（測らない。GUI・実機などで動かせないもの。reason= に理由を書く）
  filter  出力に通すシェルのパイプ（例: "grep 'error:'"、"head -n 5"）
  tty     yes なら疑似端末で動かす（シェルが出す Segmentation fault などを拾うため）

版（--distro、既定は jazzy）:
  jazzy    出力は本文にそのまま載っている。本文と比べ、--write では本文を書き換える。
  lyrical  出力は outputs/lyrical/<docs か docs-en>/<ページ>/<番号>.txt に置く（mkdocs の hook が差し込む）。
           番号は、そのページで版ごとに測る印（env が gcc か ros）を上から数えたもの。
           env=clang と env=static は版に依らないので、Lyrical では測らない。

使い方:
  python3 tools/measure.py --check              食い違いがあれば一覧を出して終了コード 1
  python3 tools/measure.py --write              測り直した出力を読み物に書き込む
  python3 tools/measure.py --check docs/c/03_ポインタ1.md   ページを絞る

毎回変わる値（アドレス・一時ファイル名・PID など）は、比べる前に正規化します。
"""

import argparse
import re
import shlex
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOC_DIRECTORIES = ("docs", "docs-en")
DEFAULT_DISTRO = "jazzy"
VERSION_DEPENDENT_ENVIRONMENTS = ("gcc", "ros")
DOCKER_PLATFORM = "linux/amd64"

MARKER = re.compile(r"^\s*<!--\s*measure:(.*?)-->\s*$")
FENCE = re.compile(r"^(\s*)```(\S*)\s*$")
# 1 行目のコメントのファイル名。`~/ros2_ws/src/pkg/a.cpp` はワークスペースからの相対パスにする。
FILE_HEADER = re.compile(r"^\s*(?://|#|<!--)\s*(?:~/ros2_ws/)?([\w./-]+\.\w+)\b")
ROS_WORKSPACE = "/home/ubuntu/ros2_ws"

# 比べる前に、実行ごとに変わる値を同じ文字列に置き換える。
VOLATILE_PATTERNS = (
    (re.compile(r"0x[0-9a-fA-F]{4,}"), "0xADDR"),
    (re.compile(r"/tmp/cc[A-Za-z0-9]+\.o"), "/tmp/ccTEMP.o"),
    (re.compile(r"==\d+=="), "==PID=="),
    (re.compile(r"\+0x[0-9a-f]+\)"), "+0xOFFSET)"),
    (re.compile(r"BuildId: [0-9a-f]+"), "BuildId: ID"),
    (re.compile(r"Testing/\d{8}-\d{4}/"), "Testing/TIMESTAMP/"),           # ctest の実行時刻
    (re.compile(r"\(\d+ ms( total)?\)"), "(N ms\\1)"),                    # gtest の所要時間
    (re.compile(r"\[\d+\.\d+s\]"), "[N.NNs]"),                            # colcon の所要時間
    (re.compile(r"\[\d{10}\.\d+\]"), "[TIMESTAMP]"),                      # ROS のログの時刻
)


@dataclass
class CodeBlock:
    start_line: int          # 開きフェンスの行番号（0 始まり）
    end_line: int            # 閉じフェンスの行番号
    language: str
    content: str
    file_name: str | None


@dataclass
class Measurement:
    page: Path
    marker_line: int
    output_block: CodeBlock
    options: dict = field(default_factory=dict)
    files: dict = field(default_factory=dict)    # ファイル名 -> 中身
    command: str = ""


def parse_code_blocks(lines):
    blocks = []
    opening = None
    for index, line in enumerate(lines):
        match = FENCE.match(line)
        if not match:
            continue
        if opening is None:
            opening = (index, match.group(2))
            continue
        start, language = opening
        content = "\n".join(lines[start + 1:index])
        blocks.append(CodeBlock(start, index, language, content, file_name_of(lines[start + 1:index])))
        opening = None
    return blocks


def file_name_of(block_lines):
    """コードブロックの先頭のコメントに書いたファイル名。XML は 1 行目が <?xml ...?> なので 2 行目まで見る。"""
    for line in block_lines[:2]:
        header = FILE_HEADER.match(line)
        if header:
            return header.group(1)
    return None


def parse_options(text):
    options = {}
    for token in shlex.split(text):
        key, _, value = token.partition("=")
        options[key.strip()] = value
    return options


def find_measurements(page):
    lines = page.read_text(encoding="utf-8").split("\n")
    blocks = parse_code_blocks(lines)
    measurements = []
    for index, line in enumerate(lines):
        match = MARKER.match(line)
        if not match:
            continue
        options = parse_options(match.group(1))
        following = [block for block in blocks if block.start_line > index]
        if not following:
            raise ValueError(f"{page}:{index + 1}: 印のあとに出力のコードブロックがありません")
        output_block = following[0]
        measurement = Measurement(page, index, output_block, options)
        if options.get("env") != "static":
            preceding = [block for block in blocks if block.end_line < index]
            # 先に出力を見せ、プログラムは後の節に載せる書き方もあるので、後ろのブロックも探す。
            later = [block for block in blocks if block.start_line > output_block.end_line]
            measurement.files = select_files(page, index, preceding, later, options.get("files"))
            measurement.command = options.get("cmd") or nearest_command(page, index, preceding)
        measurements.append(measurement)
    return measurements


def select_files(page, marker_line, preceding, later, requested):
    """使うファイル。印より前でいちばん近いものを優先し、無ければ印より後ろでいちばん近いもの。"""
    named = [block for block in preceding if block.file_name]
    if requested is None:
        if not named:
            return {}
        return {named[-1].file_name: named[-1].content}
    files = {}
    for name in requested.split(","):
        before = [block for block in named if block.file_name == name]
        after = [block for block in later if block.file_name == name]
        if before:
            files[name] = before[-1].content
        elif after:
            files[name] = after[0].content
        else:
            raise ValueError(f"{page}:{marker_line + 1}: ファイル {name} のコードブロックがページにありません")
    return files


def nearest_command(page, marker_line, preceding):
    commands = [block for block in preceding if block.language in ("bash", "sh", "shell")]
    if not commands:
        raise ValueError(f"{page}:{marker_line + 1}: 実行するコマンド（bash のコードブロックか cmd=）がありません")
    return commands[-1].content


def docker_image(distro):
    return f"ros2-drill:{distro}-amd64"


def run(measurement, distro=DEFAULT_DISTRO):
    """測った出力（末尾の空白を除く）を返す。測らない印なら None。"""
    environment = measurement.options.get("env", "gcc")
    if environment == "static":
        return None
    if distro != DEFAULT_DISTRO and environment not in VERSION_DEPENDENT_ENVIRONMENTS:
        return None
    script = measurement.command
    if measurement.options.get("tty") == "yes":
        if environment == "clang":
            # macOS（BSD）の script はコマンドを引数で受け取る。
            script = f"script -q /dev/null bash -c {shlex.quote(script)}"
        else:
            script = f"script -qec {shlex.quote(script)} /dev/null"
    script = f"( {script} ) 2>&1"
    if measurement.options.get("filter"):
        script += f" | {measurement.options['filter']}"
    with tempfile.TemporaryDirectory() as directory:
        for name, content in measurement.files.items():
            path = Path(directory) / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content + "\n", encoding="utf-8")
        if environment == "gcc":
            argv = ["docker", "run", "--rm", "--platform", DOCKER_PLATFORM,
                    "-v", f"{directory}:/w", "-w", "/w", docker_image(distro), "bash", "-c", script]
        elif environment == "ros":
            # 本文のコマンドは ~/ros2_ws で打つ前提なので、そこに置いて同じパスで動かす。
            ros_script = f"source /opt/ros/{distro}/setup.bash && cd {ROS_WORKSPACE} && {script.replace('~/ros2_ws', ROS_WORKSPACE)}"
            argv = ["docker", "run", "--rm", "--platform", DOCKER_PLATFORM,
                    "-v", f"{directory}:{ROS_WORKSPACE}", "-w", ROS_WORKSPACE, docker_image(distro), "bash", "-c", ros_script]
        elif environment == "clang":
            if sys.platform != "darwin":
                return None
            argv = ["bash", "-c", script]
        else:
            raise ValueError(f"{measurement.page}:{measurement.marker_line + 1}: 未知の env: {environment}")
        completed = subprocess.run(argv, cwd=directory, capture_output=True, text=True)
        # 125〜127 は docker や bash 自体が動けなかったときの終了コード。
        # これを出力として書き込むと本文が壊れるので、ここで止める。
        if completed.returncode in (125, 126, 127) and not completed.stdout.strip():
            raise RuntimeError(f"{measurement.page}:{measurement.marker_line + 1}: 測る仕組みが動きませんでした:\n{completed.stderr}")
        output = completed.stdout
    return "\n".join(line.rstrip() for line in output.rstrip().split("\n"))


def normalized(text):
    lines = [line.rstrip() for line in text.strip().split("\n")]
    result = "\n".join(lines)
    for pattern, replacement in VOLATILE_PATTERNS:
        result = pattern.sub(replacement, result)
    return result


def collect_pages(arguments):
    if arguments:
        return [Path(argument).resolve() for argument in arguments]
    return sorted(path for directory in DOC_DIRECTORIES for path in (ROOT / directory).rglob("*.md"))


def output_file_for(page, number, distro):
    """Lyrical などの版の出力を置くファイル。hooks/drill_version.py と同じ規則。"""
    relative = page.relative_to(ROOT)
    language_directory, page_path = relative.parts[0], Path(*relative.parts[1:]).with_suffix("")
    return ROOT / "outputs" / distro / language_directory / page_path / f"{number}.txt"


def numbered(measurements):
    """版ごとに測る印に、hook と同じ番号（1 始まり）を振る。"""
    number = 0
    for measurement in measurements:
        if measurement.options.get("env", "gcc") in VERSION_DEPENDENT_ENVIRONMENTS:
            number += 1
            yield number, measurement
        else:
            yield None, measurement


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="食い違いを検査する")
    mode.add_argument("--write", action="store_true", help="測り直した出力を書き込む")
    parser.add_argument("--distro", default=DEFAULT_DISTRO, help="ROS 2 の版（jazzy / lyrical）")
    parser.add_argument("pages", nargs="*", help="対象のページ（省略すると全部）")
    arguments = parser.parse_args()
    distro = arguments.distro

    mismatches = []
    measured_count = skipped_count = 0
    for page in collect_pages(arguments.pages):
        lines = page.read_text(encoding="utf-8").split("\n")
        changed = False
        # 後ろから書き換えれば、前の印の行番号はずれない。
        for number, measurement in reversed(list(numbered(find_measurements(page)))):
            actual = run(measurement, distro)
            if actual is None:
                skipped_count += 1
                continue
            measured_count += 1
            location = f"{page.relative_to(ROOT)}:{measurement.marker_line + 1}"
            if distro == DEFAULT_DISTRO:
                shown = measurement.output_block.content
            else:
                output_file = output_file_for(page, number, distro)
                shown = output_file.read_text(encoding="utf-8") if output_file.exists() else ""
            missing_file = distro != DEFAULT_DISTRO and not output_file.exists()
            if normalized(actual) == normalized(shown) and not missing_file:
                continue
            if not arguments.write:
                mismatches.append((location, shown, actual))
                continue
            if distro == DEFAULT_DISTRO:
                block = measurement.output_block
                lines[block.start_line + 1:block.end_line] = actual.split("\n")
                changed = True
            else:
                output_file.parent.mkdir(parents=True, exist_ok=True)
                output_file.write_text(actual + "\n", encoding="utf-8")
            print(f"書き換え: {location}")
        if changed:
            page.write_text("\n".join(lines), encoding="utf-8")

    for location, shown, actual in reversed(mismatches):
        print(f"\n✗ {location}\n--- 本文\n{shown}\n--- 実測\n{actual}")
    print(f"\n版: {distro}  測った: {measured_count}  測らなかった（static / この環境で動かせない clang / 版に依らない出力）: {skipped_count}"
          f"  食い違い: {len(mismatches)}")
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main())
