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
  - 元から混在していたファイルで、少数派の改行の行が増えた / 少数派へ一括変換された
  - 新規ファイルの中で改行が混在している
警告だけにするもの (warning):
  - 元から混在していたファイルが、多数派の改行に揃えられた (直す方向の変換)
  - 事故で変わった形式を、その前の形式へ戻した (過去の版を遡って判定する)
見ないもの:
  - バイナリ、空のファイル、シンボリックリンク、サブモジュール

意図して変換するときは、範囲内のどれかのコミットメッセージに
    Format-Change: <パス or glob>
の行を書く (複数可、* で全部)。該当ファイルの error は warning に下げる。

使い方:
    python3 .github/scripts/check-file-format.py --base <rev> --head <rev>
    python3 .github/scripts/check-file-format.py --base origin/master   # 手元で push 前に確認
                                                                        # (master との分岐点から比べる)

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

    def read(self, name):
        """name は blob の SHA か "<commit>:<path>"。無ければ None。"""
        self.proc.stdin.write(name.encode() + b"\n")
        self.proc.stdin.flush()
        header = self.proc.stdout.readline()
        # 無いときは "<name> missing"。name がパスを含むと空白が入るので、右から読む
        if header.endswith((b" missing\n", b" ambiguous\n")):
            return None
        parts = header.rsplit(b" ", 2)
        if len(parts) != 3 or parts[1] != b"blob":
            if len(parts) == 3 and parts[2].strip().isdigit():
                self.proc.stdout.read(int(parts[2]) + 1)  # blob 以外の中身を読み捨てる
            return None
        data = self.proc.stdout.read(int(parts[2]))
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
    """git diff --raw -z の出力を辞書のリストに分解する。
    キー: kind (A/C/D/M/R/T…), score (移動・複製の類似度), old_mode, new_mode, old_sha, new_sha, old_path, new_path"""
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
        entries.append({
            "kind": kind, "score": int(status[1:] or 100),
            "old_mode": old_mode, "new_mode": new_mode, "old_sha": old_sha, "new_sha": new_sha,
            "old_path": old_path, "new_path": new_path,
        })
    return entries


def normalized(data):
    """BOM と CR を除いた内容。形式だけが違う同じファイルを見分けるのに使う。"""
    if data.startswith(UTF8_BOM):
        data = data[len(UTF8_BOM):]
    return data.replace(b"\r\n", b"\n")


def pair_converted_moves(entries, reader):
    """改行を一括変換したうえで移動したファイルは、全行が変わるので git の移動検出に
    かからず、削除 + 追加に見える。BOM と CR を除いた内容が完全に一致する削除と追加が
    1 対 1 なら、移動とみなして変更前後の形式を比べる。"""
    deleted, added = {}, {}
    for e in entries:
        if e["kind"] == "D" and e["old_mode"] not in SKIP_MODES:
            deleted.setdefault(normalized(reader.read(e["old_sha"]) or b""), []).append(e)
        elif e["kind"] == "A" and e["new_mode"] not in SKIP_MODES:
            added.setdefault(normalized(reader.read(e["new_sha"]) or b""), []).append(e)
    pairs = []
    for key, adds in added.items():
        dels = deleted.get(key, [])
        if key and len(adds) == 1 and len(dels) == 1:
            d, a = dels[0], adds[0]
            pairs.append(dict(a, kind="R", old_mode=d["old_mode"], old_sha=d["old_sha"], old_path=d["old_path"]))
    return pairs


def allowed_patterns(base, head):
    """範囲内のコミットメッセージにある Format-Change: の指定を集める。"""
    if base == ZERO_SHA:
        return []
    log = git("log", "--format=%B%x00", f"{base}..{head}", check=False).decode("utf-8", "replace")
    patterns = []
    for m in FORMAT_CHANGE_RE.finditer(log):
        patterns.extend(p for p in re.split(r"[\s,]+", m.group(1)) if p)
    return patterns


def earlier_formats(reader, base, path, limit=20):
    """base 以前にこのパスを変えたコミットでの形式を、新しい順に返す (base 時点の版を含む)。"""
    out = git("log", f"-n{limit}", "--format=%H", base, "--", path, check=False).decode().split()
    formats = []
    for sha in out:
        fmt = classify(reader.read(f"{sha}:{path}"))
        if fmt is not None:
            formats.append(fmt)
    return formats


def restored(history, attr, old_value, new_value):
    """事故で変わった形式を、その前の形式へ戻しただけか。history は新しい順の過去の
    形式で、先頭が base 時点の版。master で事故を直すコミットを咎めないために使う
    (作業ブランチは master との分岐点から累積で比べるので、事故と修正が相殺される)。

    次の 2 つを満たすときだけ「戻した」とみなす。
      - 今の形式 (old) は直前の版で入ったばかりで、その前の版は new だった
      - 直近 10 版では new の方が old 以上に続いていた
    「過去のどこかで new だった」まで認めると、一度でも形式が変わったファイル
    (Nox では 80 本) で同じ事故を繰り返しても素通りする。2 つ目の条件が無いと、
    事故 → 修正 → 同じ事故、の 3 回目を「修正を戻した」と見誤る。"""
    versions = [getattr(f, attr) for f in history if f.data][:10]  # 空の版には形式が無い
    if len(versions) < 2 or versions[0] != old_value:
        return False
    return versions[1] == new_value and versions.count(new_value) >= versions.count(old_value)


def majority(fmt):
    return "CRLF" if fmt.crlf >= fmt.lf else "LF"


def compare(old, new, where, path, history):
    """変更前後の形式を比べ、(errors, warnings) を返す。history は元の形式を調べるための遅延関数。"""
    errors, warnings = [], []

    if old.bom != new.bom:
        if old.bom:
            title, message = "BOM が外れた", f"{where}: UTF-8 BOM 付きだったファイルから BOM が外れた"
        else:
            title, message = "BOM が付いた", f"{where}: BOM なしだったファイルに UTF-8 BOM が付いた"
        if restored(history(), "bom", old.bom, new.bom):
            warnings.append((path, 1, "BOM を元に戻した", f"{where}: 以前の形式 ({'BOM あり' if new.bom else 'BOM なし'}) に戻した"))
        else:
            errors.append((path, 1, title, message))

    if old.eol == new.eol and old.eol != "mixed":
        pass
    elif new.eol == "none":
        pass
    elif old.eol == "mixed":
        stray = "LF" if majority(old) == "CRLF" else "CRLF"
        old_stray = old.lf if stray == "LF" else old.crlf
        new_stray = new.lf if stray == "LF" else new.crlf
        if new.eol == majority(old) or (old.crlf == old.lf and new.eol in ("CRLF", "LF")):
            warnings.append((path, None, "改行の混在が解消された",
                f"{where}: 改行が混在していたファイルが {new.eol} に揃えられた (元 CRLF {old.crlf} 行 / LF {old.lf} 行)"))
        elif new.eol == stray:
            errors.append((path, 1, "改行コードが変わった",
                f"{where}: 改行が混在していた (CRLF {old.crlf} 行 / LF {old.lf} 行) ファイルが、少数派の {stray} に一括変換された"))
        elif new_stray > old_stray:
            lines = new.lines_ending_with(stray)
            errors.append((path, lines[0] if lines else None, "改行の混在が増えた",
                f"{where}: 多数派が {majority(old)} のファイルで {stray} の行が {old_stray} 行から {new_stray} 行に増えた"))
    elif new.eol == "mixed":
        # 元の改行と違う方で終わる行が、紛れ込んだ行
        stray = "LF" if old.eol == "CRLF" else "CRLF" if old.eol == "LF" else minority(new)
        lines = new.lines_ending_with(stray)
        errors.append((path, lines[0] if lines else None, "改行が混在した",
            f"{where}: {old.eol if old.eol != 'none' else '改行なし'} のファイルに {stray} の行が混ざった "
            f"(CRLF {new.crlf} 行 / LF {new.lf} 行、{stray} の行: {', '.join(map(str, lines))} …)"))
    elif old.eol != "none":
        if restored(history(), "eol", old.eol, new.eol):
            warnings.append((path, 1, "改行コードを元に戻した", f"{where}: 以前の改行コード {new.eol} に戻した"))
        else:
            errors.append((path, 1, "改行コードが変わった",
                f"{where}: 改行コードが {old.eol} から {new.eol} に一括変換された (差分が全行になる)"))
    return errors, warnings




def check(base, head):
    """(errors, warnings, checked) を返す。各指摘は (path, line or None, title, message)。"""
    entries = parse_raw_diff(base, head)
    reader = BlobReader()
    errors, warnings = [], []
    checked = 0
    try:
        moves = pair_converted_moves(entries, reader)
        moved_new = {e["new_path"] for e in moves}
        targets = []
        added = []
        for e in entries:
            # git が移動・複製と判定したもの (類似度 50% 以上) は同じファイルとして比べる。
            # 絞ると、移動と同時に include を直したような本物の移動での事故を見逃す。
            # 逆に、モジュールを消してテンプレートから別のモジュールを作ると別物の
            # pch.h どうしが対応付くことがある。そのときは Format-Change で通す。
            if e["kind"] in ("M", "T", "R", "C"):
                targets.append(e)
            elif e["kind"] == "A" and e["new_path"] not in moved_new:
                added.append(e)
        targets += moves

        for e in targets:
            if e["old_mode"] in SKIP_MODES or e["new_mode"] in SKIP_MODES:
                continue
            old_data = reader.read(e["old_sha"])
            new_data = reader.read(e["new_sha"])
            old, new = classify(old_data), classify(new_data)
            # 空のファイルには形式が無いので比べない。空だったファイルは新規ファイルとして見る
            if old_data is not None and not old_data:
                added.append(e)
                continue
            if old is None or new is None or not new_data:
                continue
            checked += 1
            old_path, new_path = e["old_path"], e["new_path"]
            where = new_path if old_path == new_path else f"{new_path} (移動元 {old_path})"
            cache = []

            def history(old_path=old_path, cache=cache):
                if not cache:
                    cache.append(earlier_formats(reader, base, old_path))
                return cache[0]

            errs, warns = compare(old, new, where, new_path, history)
            errors += errs
            warnings += warns

        for e in added:
            if e["new_mode"] in SKIP_MODES:
                continue
            new = classify(reader.read(e["new_sha"]))
            if new is None:
                continue
            checked += 1
            if new.eol == "mixed":
                path = e["new_path"]
                stray = minority(new)
                lines = new.lines_ending_with(stray)
                errors.append((path, lines[0] if lines else None, "改行が混在している",
                    f"{path}: 新規ファイルの中で改行が混在している (CRLF {new.crlf} 行 / LF {new.lf} 行、{stray} の行: {', '.join(map(str, lines))} …)"))
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
    # base が head の祖先でない (手元で --base origin/master を指定したが master が先へ
    # 進んでいる、force-push された、など) ときは分岐点から比べる。そうしないと、
    # こちらが触っていないファイルの master 側の変更を逆向きに検出してしまう。
    if subprocess.run(["git", "merge-base", "--is-ancestor", base, head]).returncode != 0:
        mb = git("merge-base", base, head, check=False).decode().strip()
        if mb:
            print(f"{base[:10]} は {head[:10]} の祖先ではないので、分岐点 {mb[:10]} から比べる")
            base = mb
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
