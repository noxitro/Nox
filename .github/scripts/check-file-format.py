#!/usr/bin/env python3
"""変更されたファイルの BOM と改行コードが、変更前から変わっていないかを検査する。

このリポジトリは BOM の有無も改行コード (CRLF / LF) もファイルごとに混在していて、
統一ルールでは検査できない (.gitattributes と AGENTS.md の「ファイル形式」を参照)。
守るべきなのは「各ファイルの元の形式を保つこと」なので、全体を一律に見るのではなく
base と head の 2 点を比べて、形式が変わったファイルだけを拾う。

実際に起きた事故:
  - 書き換えツールが 29 ファイルの BOM を剥がした
  - ヘッダ 1 本の改行が CRLF から LF へ一括変換され、差分が全行になった
  - CRLF のファイルに LF の行が紛れ込む (部分的な書き換え)

検出するもの (error):
  - BOM が外れた / 付いた
  - 改行コードが一括変換された (CRLF -> LF, LF -> CRLF)
  - 改行が揃っていたファイルに、別の改行の行が混ざった
  - 新規ファイルの中で改行が混在している
警告だけにするもの (warning):
  - 元から改行が混在していたファイルが、どちらかに揃えられた (直す方向の変換)

意図して変換するときは、範囲内のどれかのコミットメッセージに
    Format-Change: <パス or glob>
の行を書く (複数可、* で全部)。該当ファイルの error は warning に下げる。

使い方:
    python3 .github/scripts/check-file-format.py --base <rev> --head <rev>
    python3 .github/scripts/check-file-format.py --base origin/master   # 手元で push 前に確認

終了コード: error があれば 1、なければ 0。
"""

import argparse
import fnmatch
import os
import re
import subprocess
import sys

UTF8_BOM = b"\xef\xbb\xbf"
# git と同じく先頭 8000 バイトに NUL があればバイナリとみなす。
# UTF-16 のファイル (.rc など) もここで除外される。今のリポジトリには無い。
SNIFF_BYTES = 8000
# 指摘 1 件あたりに示す行番号の数
MAX_LINES_PER_FINDING = 5
# シンボリックリンクとサブモジュールは中身がテキストではないので見ない
SKIP_MODES = {"120000", "160000"}
ZERO_SHA = "0" * 40

FORMAT_CHANGE_RE = re.compile(r"^Format-Change:\s*(.+?)\s*$", re.MULTILINE)


def git(*args, check=True):
    return subprocess.run(["git", *args], check=check, stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout


class BlobReader:
    """git cat-file --batch を 1 プロセスだけ立てて使い回す (初回 push で数千ファイルになるため)。"""

    def __init__(self):
        self.proc = subprocess.Popen(["git", "cat-file", "--batch"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    def read(self, sha):
        self.proc.stdin.write(sha.encode() + b"\n")
        self.proc.stdin.flush()
        header = self.proc.stdout.readline().split()
        if len(header) < 3 or header[1] == b"missing":
            return None
        data = self.proc.stdout.read(int(header[2]))
        self.proc.stdout.read(1)  # 末尾の改行
        return data

    def close(self):
        self.proc.stdin.close()
        self.proc.wait()


class Format:
    """テキストファイルの BOM と改行の状態。バイナリなら None を返す (classify を参照)。"""

    def __init__(self, data):
        self.bom = data.startswith(UTF8_BOM)
        self.crlf = data.count(b"\r\n")
        self.lf = data.count(b"\n") - self.crlf
        if self.crlf == 0 and self.lf == 0:
            self.eol = "none"
        elif self.lf == 0:
            self.eol = "CRLF"
        elif self.crlf == 0:
            self.eol = "LF"
        else:
            self.eol = "mixed"
        self.data = data

    def lines_ending_with(self, eol):
        """指定の改行 (CRLF / LF) で終わる行の行番号 (1 始まり) を先頭から数件返す。"""
        result = []
        line = 1
        start = 0
        data = self.data
        while len(result) < MAX_LINES_PER_FINDING:
            pos = data.find(b"\n", start)
            if pos < 0:
                break
            is_crlf = pos > 0 and data[pos - 1:pos] == b"\r"
            if (eol == "CRLF") == is_crlf:
                result.append(line)
            line += 1
            start = pos + 1
        return result


def classify(data):
    if data is None or b"\0" in data[:SNIFF_BYTES]:
        return None
    return Format(data)


def minority(fmt):
    """混在ファイルで少ない方の改行。紛れ込んだ側として行番号を示すのに使う。"""
    return "LF" if fmt.lf <= fmt.crlf else "CRLF"


def parse_raw_diff(base, head):
    """git diff --raw -z の出力を (status, old_mode, new_mode, old_sha, new_sha, old_path, new_path) に分解する。"""
    out = git("diff", "--raw", "-z", "--no-abbrev", "-M", "--no-ext-diff", base, head)
    tokens = out.split(b"\0")
    entries = []
    i = 0
    while i < len(tokens):
        meta = tokens[i]
        if not meta.startswith(b":"):
            i += 1
            continue
        old_mode, new_mode, old_sha, new_sha, status = meta[1:].decode().split(" ")
        kind = status[0]
        if kind in ("R", "C"):
            old_path, new_path = tokens[i + 1].decode("utf-8", "replace"), tokens[i + 2].decode("utf-8", "replace")
            i += 3
        else:
            old_path = new_path = tokens[i + 1].decode("utf-8", "replace")
            i += 2
        entries.append((kind, old_mode, new_mode, old_sha, new_sha, old_path, new_path))
    return entries


def pair_renames_by_name(entries):
    """改行を一括変換したうえで移動したファイルは、内容の類似度が下がって git には
    削除 + 追加に見える。同じファイル名の削除が 1 つだけあれば、移動とみなして比べる。"""
    deleted = {}
    for e in entries:
        if e[0] == "D":
            deleted.setdefault(os.path.basename(e[5]).lower(), []).append(e)
    paired = set()
    result = []
    for e in entries:
        if e[0] != "A":
            continue
        cands = deleted.get(os.path.basename(e[6]).lower(), [])
        if len(cands) == 1 and id(cands[0]) not in paired:
            d = cands[0]
            paired.add(id(d))
            result.append(("R?", d[1], e[2], d[3], e[4], d[5], e[6]))
    return result, paired


def allowed_patterns(base, head):
    """範囲内のコミットメッセージにある Format-Change: の指定を集める。"""
    if base == ZERO_SHA:
        return []
    log = git("log", "--format=%B%x00", f"{base}..{head}", check=False).decode("utf-8", "replace")
    patterns = []
    for m in FORMAT_CHANGE_RE.finditer(log):
        patterns.extend(p for p in re.split(r"[\s,]+", m.group(1)) if p)
    return patterns


def check(base, head):
    """(errors, warnings, checked) を返す。各指摘は (path, line or None, title, message)。"""
    entries = parse_raw_diff(base, head)
    renamed, paired_deletes = pair_renames_by_name(entries)
    targets = [e for e in entries if e[0] in ("M", "T", "R", "C")]
    targets += renamed
    # 移動として扱った追加は、新規ファイルとしては見ない
    renamed_new = {e[6] for e in renamed}
    added = [e for e in entries if e[0] == "A" and e[6] not in renamed_new]

    reader = BlobReader()
    errors, warnings = [], []
    checked = 0
    try:
        for kind, old_mode, new_mode, old_sha, new_sha, old_path, new_path in targets:
            if old_mode in SKIP_MODES or new_mode in SKIP_MODES:
                continue
            old = classify(reader.read(old_sha))
            new_data = reader.read(new_sha)
            new = classify(new_data)
            if old is None or new is None or len(new_data) == 0:
                continue
            checked += 1
            where = new_path if old_path == new_path else f"{new_path} (移動元 {old_path})"

            if old.bom and not new.bom:
                errors.append((new_path, 1, "BOM が外れた", f"{where}: UTF-8 BOM 付きだったファイルから BOM が外れた"))
            elif not old.bom and new.bom:
                errors.append((new_path, 1, "BOM が付いた", f"{where}: BOM なしだったファイルに UTF-8 BOM が付いた"))

            if old.eol == new.eol or new.eol == "none":
                pass
            elif old.eol == "mixed":
                warnings.append((new_path, None, "改行の混在が解消された",
                    f"{where}: 改行が混在していたファイルが {new.eol} に揃えられた (元 CRLF {old.crlf} 行 / LF {old.lf} 行)。意図した変換なら問題ない"))
            elif new.eol == "mixed":
                # 元の改行と違う方で終わる行が、紛れ込んだ行
                stray = "LF" if old.eol == "CRLF" else "CRLF" if old.eol == "LF" else minority(new)
                lines = new.lines_ending_with(stray)
                errors.append((new_path, lines[0] if lines else None, "改行が混在した",
                    f"{where}: {old.eol if old.eol != 'none' else '改行なし'} のファイルに {stray} の行が混ざった "
                    f"(CRLF {new.crlf} 行 / LF {new.lf} 行、{stray} の行: {', '.join(map(str, lines))} …)"))
            elif old.eol != "none":
                errors.append((new_path, 1, "改行コードが変わった",
                    f"{where}: 改行コードが {old.eol} から {new.eol} に一括変換された (差分が全行になる)"))

        for kind, old_mode, new_mode, old_sha, new_sha, old_path, new_path in added:
            if new_mode in SKIP_MODES:
                continue
            new = classify(reader.read(new_sha))
            if new is None:
                continue
            checked += 1
            if new.eol == "mixed":
                stray = minority(new)
                lines = new.lines_ending_with(stray)
                errors.append((new_path, lines[0] if lines else None, "改行が混在している",
                    f"{new_path}: 新規ファイルの中で改行が混在している (CRLF {new.crlf} 行 / LF {new.lf} 行、{stray} の行: {', '.join(map(str, lines))} …)"))
    finally:
        reader.close()

    patterns = allowed_patterns(base, head)
    if patterns:
        kept = []
        for f in errors:
            if any(fnmatch.fnmatch(f[0], p) for p in patterns):
                warnings.append((f[0], f[1], f[2] + " (Format-Change で許可)", f[3]))
            else:
                kept.append(f)
        errors = kept
    return errors, warnings, checked


def escape_annotation(s):
    return s.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def escape_property(s):
    return escape_annotation(s).replace(":", "%3A").replace(",", "%2C")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base", required=True, help="比較元のリビジョン")
    ap.add_argument("--head", default="HEAD", help="比較先のリビジョン (既定: HEAD)")
    ap.add_argument("--report", help="指摘を Markdown で書き出すファイル (Discord 通知用)")
    args = ap.parse_args()

    base = git("rev-parse", "--verify", args.base + "^{commit}").decode().strip()
    head = git("rev-parse", "--verify", args.head + "^{commit}").decode().strip()
    errors, warnings, checked = check(base, head)

    in_actions = os.environ.get("GITHUB_ACTIONS") == "true"
    print(f"{base[:10]}..{head[:10]}: テキストファイル {checked} 本を検査した。error {len(errors)} 件 / warning {len(warnings)} 件")
    for level, items in (("error", errors), ("warning", warnings)):
        for path, line, title, message in items:
            if in_actions:
                loc = f"file={escape_property(path)}" + (f",line={line}" if line else "")
                print(f"::{level} {loc},title={escape_property(title)}::{escape_annotation(message)}")
            else:
                print(f"  [{level}] {message}")

    lines = []
    if errors or warnings:
        lines.append(f"`{base[:10]}..{head[:10]}` で BOM / 改行コードの変化を検出した。")
        lines.append("")
        for level, mark, items in (("error", "❌", errors), ("warning", "⚠️", warnings)):
            for path, line, title, message in items:
                lines.append(f"- {mark} **{title}** — {message}")
        if errors:
            lines.append("")
            lines.append("元の形式に戻すこと (AGENTS.md「ファイル形式」)。意図した変換なら、コミットメッセージに "
                "`Format-Change: <パス>` を書いて push し直す。")
    report = "\n".join(lines)

    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as f:
            f.write("## File format\n\n")
            f.write((report if report else f"`{base[:10]}..{head[:10]}`: 問題なし (テキストファイル {checked} 本)") + "\n")
    if args.report and errors:
        with open(args.report, "w", encoding="utf-8") as f:
            f.write(report + "\n")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
