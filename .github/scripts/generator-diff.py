#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""2 つの生成器で作った生成物 (gen/ の中身) を比べ、差分と要約を書き出す。

ci.yml の Generator diff ジョブが使う。生成器 (runtime/bin/source) を変えた push で、
変える前の生成器 (基準) と今の生成器に同じソースを解析させ、それぞれの gen/ を
<base>/<プロジェクト名>/ と <head>/<プロジェクト名>/ に写してから呼ぶ。

比べるもの:
  - 生成コード (*.g.cpp / *.g.h / gen.h など): バイト列で比べ、違えば git diff で差分を出す
  - RuntimeTypeDB.*.bin (MessagePack): バイト列で比べ、違えば宣言 1 つを 1 行にした
    テキストへ直してから差分を出す (バイナリのままでは何が変わったか読めないため)

書き出すもの (--out の下):
  - generator.diff : 全差分 (git diff の形式)
  - typedb/        : 違った TypeDB を直したテキスト (base/ と head/)
  - summary.md     : 要約。--summary を渡すと、そのファイル (GITHUB_STEP_SUMMARY) にも追記する

差分があっても終了コードは 0。生成器を変えれば生成物が変わるのは普通なので、
差分は警告 (::warning::) と要約で知らせ、意図したものかは人が読んで判断する。
比べるものが無い (生成に失敗して gen/ が空) ときだけ 1 を返す。

使い方:
    generator-diff.py --base <dir> --head <dir> --out <dir> [--title <見出し>] [--summary <path>]

TypeDB を直すのに msgpack パッケージ (pip install msgpack) を使う。差分のある TypeDB が
無ければ読み込まない。
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys

TYPEDB_PREFIX = "RuntimeTypeDB."
TYPEDB_SUFFIX = ".bin"

# TypeDB の宣言が子の宣言を持つキー。この順で子を 1 行ずつ書く
CHILD_KEYS = ("NamespaceList", "RecordList", "FunctionList", "VariableList", "EnumList")
CHILD_LABELS = {
    "NamespaceList": "namespace",
    "RecordList": "record",
    "FunctionList": "function",
    "VariableList": "variable",
    "EnumList": "enum",
}

# 要約に載せる差分の抜粋の量。全体は generator.diff を読む。
# TypeDB を直したテキストは 1 行が長い (関数 1 つで数千文字) ので、行の長さと全体の文字数も絞る。
# GITHUB_STEP_SUMMARY は 1 ステップあたり 1 MiB までで、超えると要約ごと捨てられる。
SUMMARY_DIFF_LINES = 200
SUMMARY_LINE_CHARS = 300
SUMMARY_DIFF_CHARS = 60_000

# TypeDB のフィールドの違いの表に載せる種類の数と、例の文字数
SUMMARY_FIELD_ROWS = 40
SUMMARY_EXAMPLE_CHARS = 160


def list_files(root: str) -> dict:
    """root 以下のファイルを、root からの相対パス ('/' 区切り) → 絶対パス の辞書にする"""
    files = {}
    if not os.path.isdir(root):
        return files
    for directory, _, names in os.walk(root):
        for name in names:
            path = os.path.join(directory, name)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            files[rel] = path
    return files


def sha256(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def is_typedb(rel: str) -> bool:
    name = rel.rsplit("/", 1)[-1]
    return name.startswith(TYPEDB_PREFIX) and name.endswith(TYPEDB_SUFFIX)


def dump_typedb(src: str, dst: str) -> None:
    """TypeDB (MessagePack) を、宣言 1 つを 1 行にしたテキストへ直す

    子の宣言は親の次の行から、深さの分だけ字下げして書く。宣言の並び (生成器が
    宣言を見つけた順) は変えない。並びの違いも生成物の違いなので、そのまま差分に出す。
    """
    import msgpack  # 差分のある TypeDB があるときだけ要る

    with open(src, "rb") as f:
        root = msgpack.unpackb(f.read(), raw=False, strict_map_key=False)

    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "w", encoding="utf-8", newline="\n") as out:

        def emit(label: str, node: dict, depth: int) -> None:
            own = {k: v for k, v in node.items() if k not in CHILD_KEYS}
            text = json.dumps(own, ensure_ascii=False, separators=(",", ":"))
            out.write(f"{'  ' * depth}{label} {text}\n")
            for key in CHILD_KEYS:
                for child in node.get(key) or ():
                    emit(CHILD_LABELS[key], child, depth + 1)

        for namespace in root.get("NamespaceList") or ():
            emit("namespace", namespace, 0)


def _field_diffs(a, b, path: str = ""):
    """2 つの値 (TypeDB の宣言を JSON にしたもの) の、違うフィールドを (パス, 基準の値, 今の値) で列挙する"""
    if type(a) is not type(b):
        yield path, a, b
    elif isinstance(a, dict):
        for key in list(a.keys()) + [k for k in b.keys() if k not in a]:
            if a.get(key) != b.get(key):
                yield from _field_diffs(a.get(key), b.get(key), f"{path}.{key}")
    elif isinstance(a, list):
        if len(a) != len(b):
            yield f"{path}[] の数", len(a), len(b)
        else:
            for x, y in zip(a, b):
                if x != y:
                    yield from _field_diffs(x, y, f"{path}[]")
    else:
        yield path, a, b


def summarize_typedb_patch(patch: str, counter: dict) -> None:
    """TypeDB の差分 (宣言 1 つが 1 行) を、宣言の種類とフィールドごとに数えて counter に足す

    削除行と追加行が同じ数だけ並ぶ塊は、同じ宣言の中身が変わったものとして 1 行ずつ組にし、
    違うフィールドを数える。数が合わない塊は、宣言の追加・削除・並びの変化として数える。
    counter は (宣言の種類, フィールド) → [件数, 例] の辞書。
    """

    def parse(line: str):
        label, _, text = line.strip().partition(" ")
        return label, json.loads(text)

    def add(key, example):
        entry = counter.setdefault(key, [0, example])
        entry[0] += 1

    def flush(removed: list, added: list) -> None:
        if not removed and not added:
            return
        if len(removed) != len(added):
            for line in removed:
                label, node = parse(line)
                add((label, "(宣言の削除・並びの変化)"), (node.get("FullName", ""), "", ""))
            for line in added:
                label, node = parse(line)
                add((label, "(宣言の追加・並びの変化)"), (node.get("FullName", ""), "", ""))
            return
        for before, after in zip(removed, added):
            label, a = parse(before)
            _, b = parse(after)
            for path, va, vb in _field_diffs(a, b):
                add((label, path or "(全体)"), (a.get("FullName", ""), va, vb))

    removed, added = [], []
    in_header = True
    for line in patch.splitlines():
        if line.startswith("@@"):
            in_header = False
            flush(removed, added)
            removed, added = [], []
            continue
        if in_header:
            continue
        if line.startswith("-"):
            if added:
                flush(removed, added)
                removed, added = [], []
            removed.append(line[1:])
        elif line.startswith("+"):
            added.append(line[1:])
        else:
            flush(removed, added)
            removed, added = [], []
    flush(removed, added)


def clip(value, limit: int) -> str:
    text = str(value)
    return text if len(text) <= limit else text[: limit - 1] + "…"


def git_diff(base: str, head: str, label: str) -> tuple[str, int, int]:
    """2 つのファイルの差分を git diff の形式で返す (本文, 追加行数, 削除行数)

    見出しのパスは label に置き換える (一時ディレクトリの絶対パスを出さない)。
    """
    cmd = ["git", "diff", "--no-index", "--no-color", "--no-ext-diff", "--text", "-U3", "--", base, head]
    result = subprocess.run(cmd, capture_output=True)
    if result.returncode not in (0, 1):
        raise RuntimeError(f"git diff に失敗: {result.stderr.decode('utf-8', 'replace')}")
    text = result.stdout.decode("utf-8", "replace")

    lines = []
    added = removed = 0
    in_header = True
    for line in text.splitlines():
        if line.startswith("diff --git "):
            lines.append(f"diff --git a/{label} b/{label}")
            in_header = True
            continue
        if in_header and line.startswith("--- "):
            lines.append(f"--- a/{label}")
            continue
        if in_header and line.startswith("+++ "):
            lines.append(f"+++ b/{label}")
            in_header = False
            continue
        if not in_header:
            if line.startswith("+"):
                added += 1
            elif line.startswith("-"):
                removed += 1
        lines.append(line)
    return "\n".join(lines) + ("\n" if lines else ""), added, removed


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", required=True, help="基準の生成器で作った生成物のディレクトリ")
    ap.add_argument("--head", required=True, help="今の生成器で作った生成物のディレクトリ")
    ap.add_argument("--out", required=True, help="差分と要約を書き出すディレクトリ")
    ap.add_argument("--title", default="生成物の差分", help="要約の見出し")
    ap.add_argument("--summary", default="", help="要約を追記するファイル (GITHUB_STEP_SUMMARY)")
    args = ap.parse_args()

    # Windows ランナーの標準出力は cp1252 になることがあり、日本語で UnicodeEncodeError になる
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    base_files = list_files(args.base)
    head_files = list_files(args.head)
    os.makedirs(args.out, exist_ok=True)

    if not base_files or not head_files:
        empty = [name for name, files in (("基準", base_files), ("今", head_files)) if not files]
        print(f"::error::{args.title}: 比べる生成物が無い ({'・'.join(empty)}の生成器の出力が空)")
        return 1

    # 片方にしか無いファイルは、空のファイルと比べて中身を差分に出す
    empty = os.path.join(args.out, "empty")
    open(empty, "wb").close()

    rows = []  # (相対パス, 状態, 追加, 削除)
    patches = []
    unchanged = 0
    typedb_fields = {}  # (宣言の種類, フィールド) → [件数, 例]
    for rel in sorted(set(base_files) | set(head_files)):
        base_path = base_files.get(rel)
        head_path = head_files.get(rel)
        if base_path is None or head_path is None:
            state = "追加" if base_path is None else "削除"
            if is_typedb(rel):
                rows.append((rel, state, "", ""))
                continue
            patch, added, removed = git_diff(base_path or empty, head_path or empty, rel)
            rows.append((rel, state, str(added), str(removed)))
            patches.append(patch)
            continue
        if os.path.getsize(base_path) == os.path.getsize(head_path) and sha256(base_path) == sha256(head_path):
            unchanged += 1
            continue

        if is_typedb(rel):
            text_rel = f"typedb/{rel}.txt"
            base_text = os.path.join(args.out, "typedb", "base", rel + ".txt")
            head_text = os.path.join(args.out, "typedb", "head", rel + ".txt")
            dump_typedb(base_path, base_text)
            dump_typedb(head_path, head_text)
            patch, added, removed = git_diff(base_text, head_text, text_rel)
            if not patch:
                # 宣言の中身は同じで、MessagePack の符号化だけが違う
                rows.append((rel, "変更 (符号化だけ)", "0", "0"))
                continue
            summarize_typedb_patch(patch, typedb_fields)
        else:
            patch, added, removed = git_diff(base_path, head_path, rel)
        rows.append((rel, "変更", str(added), str(removed)))
        patches.append(patch)

    diff_path = os.path.join(args.out, "generator.diff")
    with open(diff_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(patches))

    lines = [f"## {args.title}", ""]
    compared = len(set(base_files) | set(head_files))
    if not rows:
        lines.append(f":white_check_mark: 差分なし (比べたファイル {compared} 個)")
    else:
        lines.append(f":warning: 差分あり: {len(rows)} 個 (同じだったもの {unchanged} 個)")
        lines.append("")
        lines.append("| ファイル | 状態 | + | - |")
        lines.append("|---|---|---:|---:|")
        for rel, state, added, removed in rows:
            lines.append(f"| `{rel}` | {state} | {added} | {removed} |")
        if typedb_fields:
            # 違いを宣言の種類とフィールドで数えた表。TypeDB の差分は行が長く、抜粋だけでは何が変わったか読めないため
            lines.append("")
            lines.append("TypeDB の違い (宣言の種類とフィールドごとの件数。例は最初に見つかったもの):")
            lines.append("")
            lines.append("| 宣言 | フィールド | 件数 | 例 (宣言 / 基準 → 今) |")
            lines.append("|---|---|---:|---|")
            ordered = sorted(typedb_fields.items(), key=lambda item: -item[1][0])
            for (label, path), (count, (name, before, after)) in ordered[:SUMMARY_FIELD_ROWS]:
                example = f"`{clip(name, SUMMARY_EXAMPLE_CHARS)}`"
                if before != "" or after != "":
                    example += f" / `{clip(before, SUMMARY_EXAMPLE_CHARS)}` → `{clip(after, SUMMARY_EXAMPLE_CHARS)}`"
                lines.append(f"| {label} | `{path}` | {count} | {example.replace('|', '&#124;')} |")
            if len(ordered) > SUMMARY_FIELD_ROWS:
                lines.append(f"| … | ほか {len(ordered) - SUMMARY_FIELD_ROWS} 種類 | | |")
        all_lines = "".join(patches).splitlines()
        if all_lines:
            shown = []
            size = 0
            for line in all_lines[:SUMMARY_DIFF_LINES]:
                line = clip(line, SUMMARY_LINE_CHARS)
                size += len(line) + 1
                if size > SUMMARY_DIFF_CHARS:
                    break
                shown.append(line)
            lines.append("")
            lines.append(f"<details><summary>差分 (先頭 {len(shown)} / {len(all_lines)} 行、長い行は切り詰め。全体はアーティファクトの generator.diff)</summary>")
            lines.append("")
            lines.append("```diff")
            lines.extend(shown)
            lines.append("```")
            lines.append("")
            lines.append("</details>")
        print(f"::warning::{args.title}: 生成物が {len(rows)} 個違う。意図した差分か、要約とアーティファクトの generator.diff で確かめること")

    summary = "\n".join(lines) + "\n"
    with open(os.path.join(args.out, "summary.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write(summary)
    if args.summary:
        with open(args.summary, "a", encoding="utf-8") as f:
            f.write(summary)
    print(summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
