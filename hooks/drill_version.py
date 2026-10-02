"""読み物を ROS 2 の版（Jazzy / Lyrical）に合わせて組み立てる mkdocs の hook。

本文は 1 つで、Jazzy（既定の版）の出力をそのまま載せています。Lyrical 版を建てるときだけ、
この hook が次の 3 つを差し替えます。

1. 測った出力: `<!-- measure: ... -->` の印の直後の出力ブロックを、
   outputs/<版>/<docs か docs-en>/<ページのパス>/<番号>.txt に差し替える。
   番号は、そのページで「版ごとに測る印」（env が gcc か ros のもの）を上から数えたもの（1 始まり）。
   env=clang（Apple clang）と env=static は版に依らないので差し替えない。
2. 版だけで変わる語: mkdocs の設定の extra.drill_replacements（[置き換え前, 置き換え後] の並び）で置き換える。
3. 版ごとの文: `<!-- only: jazzy -->` 〜 `<!-- /only -->` で囲んだ部分は、その版のときだけ残す。

Jazzy 版（extra.drill_distro が jazzy か未設定）では、3 の「ほかの版だけの部分」を消すだけです。
"""

import re
from pathlib import Path

from mkdocs.exceptions import PluginError

DEFAULT_DISTRO = "jazzy"
VERSION_DEPENDENT_ENVIRONMENTS = ("gcc", "ros")

MARKER = re.compile(r"^\s*<!--\s*measure:(.*?)-->\s*$")
FENCE = re.compile(r"^\s*```")
ENVIRONMENT = re.compile(r"\benv=(\S+)")
ONLY_BLOCK = re.compile(r"<!--\s*only:\s*(\w+)\s*-->\n?(.*?)<!--\s*/only\s*-->\n?", re.S)


def on_page_markdown(markdown, page, config, files):
    distro = config["extra"].get("drill_distro", DEFAULT_DISTRO)
    markdown = keep_only_blocks_for(distro, markdown)
    if distro == DEFAULT_DISTRO:
        return markdown
    markdown = replace_measured_outputs(markdown, distro, page, config)
    for before, after in config["extra"].get("drill_replacements", []):
        markdown = markdown.replace(before, after)
    return markdown


def keep_only_blocks_for(distro, markdown):
    def choose(match):
        return match.group(2) if match.group(1) == distro else ""
    return ONLY_BLOCK.sub(choose, markdown)


def replace_measured_outputs(markdown, distro, page, config):
    """版ごとに測る印の直後の出力ブロックを、その版で測ったファイルの中身にする。"""
    root = Path(config["config_file_path"]).parent
    language_directory = Path(config["docs_dir"]).name          # docs か docs-en
    output_directory = root / "outputs" / distro / language_directory / Path(page.file.src_uri).with_suffix("")
    lines = markdown.split("\n")
    result = []
    number = 0
    index = 0
    while index < len(lines):
        line = lines[index]
        result.append(line)
        marker = MARKER.match(line)
        index += 1
        if not marker:
            continue
        environment = ENVIRONMENT.search(marker.group(1))
        if environment and environment.group(1) not in VERSION_DEPENDENT_ENVIRONMENTS:
            continue
        number += 1
        if index >= len(lines) or not FENCE.match(lines[index]):
            raise PluginError(f"{page.file.src_uri}: measure の印の直後に出力ブロックがありません")
        output_file = output_directory / f"{number}.txt"
        if not output_file.exists():
            raise PluginError(f"{page.file.src_uri}: {distro} 版の出力がありません: {output_file.relative_to(root)}"
                              f"（python3 tools/measure.py --write --distro {distro} で作ります）")
        result.append(lines[index])                                 # 開きフェンス
        closing = index + 1
        while closing < len(lines) and not FENCE.match(lines[closing]):
            closing += 1
        result.extend(output_file.read_text(encoding="utf-8").rstrip("\n").split("\n"))
        result.append(lines[closing])                               # 閉じフェンス
        index = closing + 1
    return "\n".join(result)
