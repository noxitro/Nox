#!/usr/bin/env python3
"""MSBuild のログから clang-tidy の指摘を拾い、集計して前回と比べる。

.github/workflows/clang-tidy.yml から呼ばれる。やること:
  1. ログから clang-tidy の警告行を拾う (同じヘッダの指摘は翻訳単位の数だけ
     重複して出るので、ファイル・行・桁・検査名・文面で重複を除く)
  2. 検査名ごと・ファイルごとに数える
  3. 前回の結果 (--previous) と比べ、新しく出た指摘を特定する。行番号は前後の
     編集でずれるので、比べるときは (ファイル, 検査名, 文面) を指紋にする
  4. ジョブのサマリー、全件の JSON、Discord 用の本文を書き出す

報告だけで失敗にはしない。ただしログに clang-tidy が走った形跡が無ければ
終了コード 1 (設定が効いておらず、0 件を「きれい」と誤読するのを防ぐ)。

使い方:
    python3 .github/scripts/clang-tidy-report.py --log build.log --root . \\
        --previous prev.json --out current.json --discord discord.md
"""

import argparse
import collections
import json
import os
import re
import sys

# MSBuild の警告行。先頭の "12>" (並列ビルドのプロジェクト番号) と、末尾の
# " [C:\...\foo.vcxproj]" (どのプロジェクトのビルド中か) が付くことがある。
#   C:\a\Nox\runtime\core\x.cpp(12,5): warning : 文面 [bugprone-foo] [C:\...\core.vcxproj]
#   C:\a\Nox\runtime\core\x.cpp:12:5: warning: 文面 [bugprone-foo]
LINE_RE = re.compile(
    r"^\s*(?:\d+>)?\s*(?P<file>(?:[A-Za-z]:)?[^():\n]+?)"
    r"(?:\((?P<line1>\d+)(?:,(?P<col1>\d+))?\)\s*:|:(?P<line2>\d+):(?P<col2>\d+):)"
    r"\s*(?P<level>warning|error)\s*[A-Za-z0-9]*\s*:\s*(?P<msg>.*?)"
    r"\s*\[(?P<check>[a-z][A-Za-z0-9_.-]*(?:,[a-z][A-Za-z0-9_.-]*)*)\]"
    r"(?:\s*\[[^\]]*\.vcxproj\])?\s*$"
)
# clang-tidy が走ったことを示す行 (MSBuild の ClangTidy タスクや clang-tidy 本体の出力)
RAN_RE = re.compile(r"clang-tidy", re.IGNORECASE)


def normalize_path(path, root):
    path = path.strip().replace("\\", "/")
    root = os.path.abspath(root).replace("\\", "/").rstrip("/") + "/"
    # Windows のランナーではドライブ文字の大小が揃わないことがある
    if path.lower().startswith(root.lower()):
        path = path[len(root):]
    else:
        # ルートの外 (ランナーの作業ディレクトリ名が違う等) でも runtime/ 以下に揃える
        idx = path.lower().find("/runtime/")
        if idx >= 0:
            path = path[idx + 1:]
    return path


def parse(log_paths, root):
    findings = {}
    ran = False
    for log_path in log_paths:
        with open(log_path, encoding="utf-8", errors="replace") as f:
            for raw in f:
                if not ran and RAN_RE.search(raw):
                    ran = True
                m = LINE_RE.match(raw.rstrip("\r\n"))
                if not m:
                    continue
                checks = m.group("check")
                # コンパイラ自身の警告 ([-Wfoo]) や、clang-tidy が転載するコンパイラ診断は除く
                if checks.startswith("clang-diagnostic-"):
                    continue
                path = normalize_path(m.group("file"), root)
                line = int(m.group("line1") or m.group("line2"))
                col = int(m.group("col1") or m.group("col2") or 0)
                msg = m.group("msg").strip()
                key = (path, line, col, checks, msg)
                findings.setdefault(key, {
                    "file": path, "line": line, "col": col, "check": checks,
                    "message": msg, "level": m.group("level"),
                })
    return list(findings.values()), ran


def fingerprint(f):
    return f"{f['file']}|{f['check']}|{f['message']}"


def summarize(findings):
    by_check = collections.Counter()
    by_file = collections.Counter()
    for f in findings:
        for check in f["check"].split(","):
            by_check[check] += 1
        by_file[f["file"]] += 1
    return by_check, by_file


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--log", action="append", required=True, help="MSBuild のログ (複数可)")
    ap.add_argument("--root", default=".", help="リポジトリのルート (パスを相対にするため)")
    ap.add_argument("--previous", help="前回の結果 (JSON)。無ければ比較しない")
    ap.add_argument("--out", required=True, help="今回の結果を書き出す JSON")
    ap.add_argument("--discord", help="Discord に投稿する本文を書き出すファイル")
    ap.add_argument("--run-url", default="", help="この実行の URL (本文に載せる)")
    args = ap.parse_args()

    findings, ran = parse(args.log, args.root)
    if not ran:
        print("::error::ログに clang-tidy が走った形跡が無い。EnableClangTidyCodeAnalysis が効いていないか、"
              "clang-tidy が見つかっていない")
        return 1

    findings.sort(key=lambda f: (f["file"], f["line"], f["col"], f["check"]))
    by_check, by_file = summarize(findings)

    previous = None
    if args.previous and os.path.exists(args.previous):
        try:
            with open(args.previous, encoding="utf-8") as f:
                previous = json.load(f)
        except (OSError, ValueError) as e:
            print(f"::warning::前回の結果を読めなかった ({e})。比較しない")

    prev_by_check = collections.Counter(previous.get("by_check", {})) if previous else collections.Counter()
    prev_prints = collections.Counter(previous.get("fingerprints", {})) if previous else collections.Counter()
    # 指紋ごとの件数で比べる (同じ文面の指摘が 1 ファイルに複数あっても、増えた分だけを新規とする)
    cur_prints = collections.Counter(fingerprint(f) for f in findings)
    new_findings = []
    budget = cur_prints - prev_prints
    for f in findings:
        fp = fingerprint(f)
        if previous is not None and budget[fp] > 0:
            budget[fp] -= 1
            new_findings.append(f)
    fixed_count = sum((prev_prints - cur_prints).values()) if previous else 0

    total = len(findings)
    prev_total = previous.get("total") if previous else None
    delta = "" if prev_total is None else f" (前回 {prev_total} 件、{'+' if total >= prev_total else ''}{total - prev_total})"
    print(f"clang-tidy: {total} 件{delta} / 新規 {len(new_findings)} 件 / 解消 {fixed_count} 件")

    with open(args.out, "w", encoding="utf-8") as f:
        json.dump({
            "total": total,
            "by_check": dict(by_check),
            "fingerprints": dict(cur_prints),
            "findings": findings,
        }, f, ensure_ascii=False, indent=1)

    def check_rows(limit):
        rows = []
        for check, n in by_check.most_common(limit):
            d = n - prev_by_check.get(check, 0)
            mark = "" if previous is None or d == 0 else f" ({'+' if d > 0 else ''}{d})"
            rows.append((check, n, mark))
        return rows

    # ジョブのサマリー
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as f:
            f.write(f"## clang-tidy: {total} 件{delta}\n\n")
            if previous is not None:
                f.write(f"新規 {len(new_findings)} 件 / 解消 {fixed_count} 件\n\n")
            f.write("| 検査 | 件数 |\n|---|---|\n")
            for check, n, mark in check_rows(40):
                f.write(f"| `{check}` | {n}{mark} |\n")
            f.write("\n### 件数の多いファイル\n\n| ファイル | 件数 |\n|---|---|\n")
            for path, n in by_file.most_common(20):
                f.write(f"| `{path}` | {n} |\n")
            if new_findings:
                f.write("\n### 新しく出た指摘\n\n")
                for x in new_findings[:100]:
                    f.write(f"- `{x['file']}:{x['line']}` {x['message']} `[{x['check']}]`\n")
                if len(new_findings) > 100:
                    f.write(f"- …ほか {len(new_findings) - 100} 件 (全件はアーティファクトの JSON)\n")

    # Discord の本文
    if args.discord:
        lines = []
        if previous is not None:
            lines.append(f"**新規 {len(new_findings)} 件 / 解消 {fixed_count} 件**")
            lines.append("")
        lines.append("**検査ごとの件数 (上位)**")
        for check, n, mark in check_rows(12):
            lines.append(f"`{check}` {n}{mark}")
        if new_findings:
            lines += ["", "**新しく出た指摘**"]
            for x in new_findings[:15]:
                lines.append(f"`{x['file']}:{x['line']}` {x['message'][:120]} `[{x['check']}]`")
            if len(new_findings) > 15:
                lines.append(f"…ほか {len(new_findings) - 15} 件")
        if args.run_url:
            lines += ["", f"詳細: {args.run_url}"]
        with open(args.discord, "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")

    out = os.environ.get("GITHUB_OUTPUT")
    if out:
        with open(out, "a", encoding="utf-8") as f:
            f.write(f"total={total}\nnew={len(new_findings)}\nfixed={fixed_count}\n")
            f.write(f"compared={'true' if previous is not None else 'false'}\n")

    # 新しく出た指摘は Actions の画面で該当行に注釈として出す (多すぎると埋もれるので上限つき)
    for x in new_findings[:50]:
        print(f"::warning file={x['file']},line={x['line']},col={x['col']},title={x['check']}::{x['message']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
