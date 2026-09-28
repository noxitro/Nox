#!/usr/bin/env python3
"""外部資料 (発表・書籍・他社の製品や設計) の名前がリポジトリへ入るのを止める。

名前そのものをこのリポジトリに書くと、検査の仕組みが漏えい元になる。
そのため禁止する名前は external-names.sha256 にハッシュで置き、
本文から切り出した候補語をハッシュして突き合わせる。

候補語の作り方:
  NFKC 正規化して小文字にし、英数字の並びとカタカナの並びを語として取り出す。
  間が区切り 2 文字以内 (空白 - _ . / ・) で続く語は最大 3 語までつなげて
  1 つの候補にもする。"Foo-Bar" "foo_bar" "FooBar" "foo bar" はどれも
  "foobar" になる。

ハッシュは総当たりで元に戻せる。目的は「名前を平文で置かない」ことであって、
秘密にすることではない。

使い方:
  check-external-names.py --blobs          標準入力の "<path>\\t<blob-ish>" を検査 (scan.sh と同じ形式)
  check-external-names.py --message FILE   コミットメッセージを検査 (commit-msg フック)
  check-external-names.py --commits REV... 範囲内のコミットメッセージを検査 (pre-push / CI)
  check-external-names.py --hash NAME...   禁止リストへ足す行を出力する

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
import unicodedata

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
GAP_RE = re.compile(r"[\s\-_./・]{0,2}")


def digest(word: str) -> str:
    return hashlib.sha256((SALT + word).encode("utf-8")).hexdigest()


def normalize(text: str) -> str:
    return unicodedata.normalize("NFKC", text).lower()


def load_hashes() -> set[str]:
    hashes: set[str] = set()
    for line in LIST_PATH.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            hashes.add(line.lower())
    return hashes


def mask(word: str) -> str:
    return word[:1] + "*" * (len(word) - 1)


def find_in_line(line: str, hashes: set[str]) -> str | None:
    """行の中で最初に見つかった禁止語 (伏せ字) を返す。"""
    norm = normalize(line)
    for term in HINT_TERMS:
        if term in norm:
            return term
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


def scan_text(label: str, text: str, hashes: set[str]) -> int:
    found = 0
    for no, line in enumerate(text.splitlines(), 1):
        hit = find_in_line(line, hashes)
        if hit:
            print(f"  [EXTERNAL-NAME] {label}:{no}\n    {hit} — 外部資料の名前 / 外部資料への言及", file=sys.stderr)
            found += 1
    return found


def read_blobs(pairs: list[tuple[str, str]]) -> list[tuple[str, bytes]]:
    """git cat-file --batch 一発でまとめて読む。"""
    if not pairs:
        return []
    stdin = "".join(obj + "\n" for _, obj in pairs).encode("utf-8")
    out = subprocess.run(["git", "cat-file", "--batch"], input=stdin, capture_output=True, check=True).stdout
    result: list[tuple[str, bytes]] = []
    pos = 0
    for path, _ in pairs:
        nl = out.index(b"\n", pos)
        header = out[pos:nl].split()
        pos = nl + 1
        if len(header) < 3 or header[1] == b"missing":
            continue
        size = int(header[2])
        body = out[pos:pos + size]
        pos += size + 1
        if header[1] == b"blob":
            result.append((path, body))
    return result


def cmd_blobs(hashes: set[str]) -> int:
    pairs = []
    for line in sys.stdin.read().splitlines():
        path, sep, obj = line.partition("\t")
        if sep and path and path not in SELF_PATHS:
            pairs.append((path, obj))
    found = 0
    for path, body in read_blobs(pairs):
        if len(body) > MAX_BYTES or b"\0" in body[:8192]:
            continue
        found += scan_text(path, body.decode("utf-8", errors="replace"), hashes)
    return found


def cmd_message(path: str, hashes: set[str]) -> int:
    text = pathlib.Path(path).read_text(encoding="utf-8", errors="replace")
    # git commit -v の差分や # で始まるコメントは記録されないので見ない。
    text = text.split("# ------------------------ >8 ------------------------", 1)[0]
    text = "\n".join(line for line in text.splitlines() if not line.startswith("#"))
    return scan_text("コミットメッセージ", text, hashes)


def cmd_commits(rev_range: list[str], hashes: set[str]) -> int:
    out = subprocess.run(["git", "log", "--format=%H%x00%B%x01", *rev_range],
                         capture_output=True, check=True).stdout.decode("utf-8", errors="replace")
    found = 0
    for entry in out.split("\x01"):
        sha, sep, body = entry.strip("\n").partition("\x00")
        if sep:
            found += scan_text(f"コミット {sha[:10]} のメッセージ", body, hashes)
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--blobs", action="store_true")
    group.add_argument("--message")
    group.add_argument("--commits", nargs=argparse.REMAINDER, metavar="REV")
    group.add_argument("--hash", nargs="+", metavar="NAME")
    args = parser.parse_args()

    if args.hash:
        for name in args.hash:
            word = "".join(t.group() for t in TOKEN_RE.finditer(normalize(name)))
            print(digest(word))
        return 0

    hashes = load_hashes()
    if args.blobs:
        found = cmd_blobs(hashes)
    elif args.message:
        found = cmd_message(args.message, hashes)
    else:
        found = cmd_commits(args.commits, hashes)

    if found:
        print(
            "\n  外部資料の名前は書かないこと。設計の説明は資料名を出さずに自分の言葉で書く。\n"
            "  コミットメッセージなら書き直してから push する (git commit --amend / rebase)。",
            file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
