#!/usr/bin/env python3
"""外部資料 (発表・書籍・他社の製品や設計) の名前がリポジトリへ入るのを止める。

名前そのものをこのリポジトリに書くと、検査の仕組みが漏えい元になる。
そのため禁止する名前は external-names.sha256 にハッシュで置き、
本文から切り出した候補語をハッシュして突き合わせる。

照合は 2 種類ある。どちらも NFKC 正規化して小文字にしてから行う。

  語として照合 (名前が英数字とカタカナだけのとき。リストの行はハッシュのみ):
    英数字の並びとカタカナの並びを語として取り出す。間が区切り 2 文字以内
    (空白 - _ . / ・) で続く語は最大 3 語までつなげて 1 つの候補にもする。
    "Foo-Bar" "foo_bar" "FooBar" "foo bar" はどれも "foobar" になる。
    語の境界を見るので、長い単語の一部に偶然含まれても誤検出しない。

  部分文字列として照合 (漢字・ひらがなを含むとき。リストの行は "ハッシュ 文字数"):
    日本語には語の区切りが無いので、漢字・ひらがなを含む行だけを対象に、
    文字以外 (空白・記号) を除いた並びから登録済みの文字数の窓を切り出して照合する。

  登録 (--hash) と検出は同じ正規化 (compact) を使う。

ハッシュは総当たりで元に戻せる。目的は「名前を平文で置かない」ことであって、
秘密にすることではない。

使い方:
  check-external-names.py --blobs          標準入力の "<path>\\t<blob-ish>" を検査 (scan.sh と同じ形式)
  check-external-names.py --message FILE   コミットメッセージを検査 (commit-msg フック)
  check-external-names.py --commits REV... 範囲内のコミットメッセージを検査 (pre-push / CI)
  check-external-names.py --hash NAME...   禁止リストへ足す行を出力する

  --list FILE で禁止リストを指定できる (複数可)。省略時は隣の external-names.sha256。

検出したら場所を表示して終了コード 1 を返す。一致した語は伏せ字で出す
(CI のログは公開されるため)。
"""
from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import subprocess
import sys
import threading
import unicodedata
from collections.abc import Iterator

HERE = pathlib.Path(__file__).resolve().parent
LIST_PATH = HERE / "external-names.sha256"
SALT = "nox-external-name:"
MAX_BYTES = 5 * 1024 * 1024
MAX_JOIN = 3

# 名前ではないが、外部資料からの引き写しを示す語。平文で置いてよい。
HINT_TERMS = ("講演",)

# 検査の仕組み自身。HINT_TERMS を平文で持つので対象から外す。
SELF_PATHS = ("tools/git-hooks/check-external-names.py", "tools/git-hooks/external-names.sha256")

TOKEN_RE = re.compile(r"[0-9a-z]+|[ァ-ヺー]+")
TOKEN_ONLY_RE = re.compile(r"(?:[0-9a-z]|[ァ-ヺー])+")
GAP_RE = re.compile(r"[\s\-_./・]{0,2}")
# 部分文字列照合を行う行の目印 (ひらがな・漢字)。
CJK_RE = re.compile(r"[ぁ-ゖ\u3400-\u9fff\uf900-\ufaff]")


class NameList:
    """禁止リスト。語として照合するものと、部分文字列として照合するもの。"""

    def __init__(self) -> None:
        self.tokens: set[str] = set()
        self.substrings: set[str] = set()
        self.lengths: set[int] = set()

    def load(self, path: pathlib.Path) -> None:
        for line in path.read_text(encoding="utf-8").splitlines():
            fields = line.split("#", 1)[0].split()
            if not fields:
                continue
            if len(fields) == 1:
                self.tokens.add(fields[0].lower())
            else:
                self.substrings.add(fields[0].lower())
                self.lengths.add(int(fields[1]))


def digest(word: str) -> str:
    return hashlib.sha256((SALT + word).encode("utf-8")).hexdigest()


def normalize(text: str) -> str:
    return unicodedata.normalize("NFKC", text).lower()


def compact(norm: str) -> str:
    """正規化済みの文字列から文字 (英数字・かな・漢字) 以外を除く。登録と検出で共有する。"""
    return "".join(ch for ch in norm if ch.isalnum())


def list_entry(name: str) -> str:
    """--hash の出力 1 行。"""
    word = compact(normalize(name))
    if not word:
        raise ValueError(f"文字を含まない名前は登録できない: {name!r}")
    if TOKEN_ONLY_RE.fullmatch(word):
        return digest(word)
    return f"{digest(word)} {len(word)}"


def mask(word: str) -> str:
    return word[:1] + "*" * (len(word) - 1)


def find_in_line(line: str, names: NameList) -> str | None:
    """行の中で最初に見つかった禁止語 (伏せ字) を返す。"""
    norm = normalize(line)
    for term in HINT_TERMS:
        if term in norm:
            return term
    if names.substrings and CJK_RE.search(norm):
        text = compact(norm)
        for n in names.lengths:
            for i in range(len(text) - n + 1):
                window = text[i:i + n]
                if digest(window) in names.substrings:
                    return mask(window)
    hashes = names.tokens
    tokens = list(TOKEN_RE.finditer(norm))
    for i, first in enumerate(tokens):
        word = first.group()
        end = first.end()
        for j in range(i, min(i + MAX_JOIN, len(tokens))):
            if j > i:
                nxt = tokens[j]
                if not GAP_RE.fullmatch(norm, end, nxt.start()):
                    break
                word += nxt.group()
                end = nxt.end()
            if len(word) >= 4 and digest(word) in hashes:
                return mask(word)
    return None


def scan_text(label: str, text: str, names: NameList) -> int:
    found = 0
    for no, line in enumerate(text.splitlines(), 1):
        hit = find_in_line(line, names)
        if hit:
            print(f"  [EXTERNAL-NAME] {label}:{no}\n    {hit} — 外部資料の名前 / 外部資料への言及", file=sys.stderr)
            found += 1
    return found


def iter_blobs(pairs: list[tuple[str, str]]) -> Iterator[tuple[str, bytes]]:
    """git cat-file --batch 1 本で順に読む。

    出力は 1 blob ずつ読み出して手放すので、初回 push で全ファイルが流れてきても
    同時に抱えるのは 1 blob 分だけになる。上限を超える blob は読み飛ばして本文を持たない。
    """
    if not pairs:
        return
    proc = subprocess.Popen(["git", "cat-file", "--batch"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    assert proc.stdin is not None and proc.stdout is not None

    # 入力を書き切る前に出力のパイプが詰まると互いに待つので、書き込みは別スレッドで行う。
    def feed() -> None:
        try:
            for _, obj in pairs:
                proc.stdin.write(obj.encode("utf-8") + b"\n")
        finally:
            proc.stdin.close()

    writer = threading.Thread(target=feed, daemon=True)
    writer.start()
    out = proc.stdout
    for path, _ in pairs:
        header = out.readline().split()
        if not header:
            break
        if len(header) < 3 or header[1] == b"missing":
            continue
        size = int(header[2])
        if header[1] != b"blob" or size > MAX_BYTES:
            while size > 0:
                size -= len(out.read(min(size, 1 << 20)))
            out.read(1)
            continue
        body = out.read(size)
        out.read(1)
        yield path, body
    writer.join()
    out.close()
    proc.wait()


def cmd_blobs(names: NameList) -> int:
    pairs = []
    for line in sys.stdin.read().splitlines():
        path, sep, obj = line.partition("\t")
        if sep and path and path not in SELF_PATHS:
            pairs.append((path, obj))
    found = 0
    for path, body in iter_blobs(pairs):
        if b"\0" in body[:8192]:
            continue
        found += scan_text(path, body.decode("utf-8", errors="replace"), names)
    return found


def cmd_message(path: str, names: NameList) -> int:
    text = pathlib.Path(path).read_text(encoding="utf-8", errors="replace")
    # git commit -v の差分や # で始まるコメントは記録されないので見ない。
    text = text.split("# ------------------------ >8 ------------------------", 1)[0]
    text = "\n".join(line for line in text.splitlines() if not line.startswith("#"))
    return scan_text("コミットメッセージ", text, names)


def cmd_commits(rev_range: list[str], names: NameList) -> int:
    out = subprocess.run(["git", "log", "--format=%H%x00%B%x01", *rev_range],
                         capture_output=True, check=True).stdout.decode("utf-8", errors="replace")
    found = 0
    for entry in out.split("\x01"):
        sha, sep, body = entry.strip("\n").partition("\x00")
        if sep:
            found += scan_text(f"コミット {sha[:10]} のメッセージ", body, names)
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--blobs", action="store_true")
    group.add_argument("--message")
    group.add_argument("--commits", nargs=argparse.REMAINDER, metavar="REV")
    group.add_argument("--hash", nargs="+", metavar="NAME")
    parser.add_argument("--list", action="append", type=pathlib.Path, metavar="FILE")
    args = parser.parse_args()

    if args.hash:
        try:
            for name in args.hash:
                print(list_entry(name))
        except ValueError as e:
            print(e, file=sys.stderr)
            return 2
        return 0

    names = NameList()
    for path in args.list or [LIST_PATH]:
        names.load(path)
    if args.blobs:
        found = cmd_blobs(names)
    elif args.message:
        found = cmd_message(args.message, names)
    else:
        found = cmd_commits(args.commits, names)

    if found:
        print(
            "\n  外部資料の名前は書かないこと。設計の説明は資料名を出さずに自分の言葉で書く。\n"
            "  コミットメッセージなら書き直してから push する (git commit --amend / rebase)。",
            file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
